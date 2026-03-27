"""
Email client module for connecting to ProtonMail Bridge via IMAP.
"""

import imaplib
import email
import os
from email.header import decode_header
from email.utils import parsedate_to_datetime
from pathlib import Path
from typing import Optional, List, Tuple

from dotenv import load_dotenv

from .db import insert_email, insert_attachment, update_last_uid, get_last_uid


BRIDGE_HOST = os.environ.get("PROTON_EMAIL_BRIDGE_HOST", "localhost")
BRIDGE_PORT = int(os.environ.get("PROTON_EMAIL_BRIDGE_PORT", "1143"))


def get_credentials():
    """Read credentials from .env file, path can be overridden via environment variable."""
    env_path = os.environ.get("PROTON_EMAIL_ENV_PATH", Path(__file__).parent.parent.parent / ".env")
    load_dotenv(env_path)

    email_addr = os.environ.get("PROTONMAIL_EMAIL")
    password = os.environ.get("PROTONMAIL_PASSWORD")

    if not email_addr or not password:
        raise ValueError(
            "Set PROTONMAIL_EMAIL and PROTONMAIL_PASSWORD in the .env file "
            "(in parent directory)"
        )

    return email_addr, password


def connect():
    """Connect to ProtonMail Bridge IMAP server."""
    email_addr, password = get_credentials()
    mail = imaplib.IMAP4(host=BRIDGE_HOST, port=BRIDGE_PORT)
    mail.login(email_addr, password)
    return mail


def decode_email_header(header: str) -> str:
    """Decode an email header that may be encoded."""
    if not header:
        return ""
    decoded_parts = decode_header(header)
    result = ""
    for part, charset in decoded_parts:
        if isinstance(part, bytes):
            charset = charset or "utf-8"
            result += part.decode(charset, errors="replace")
        else:
            result += part
    return result


def parse_email_body(msg) -> Tuple[str, str]:
    """Extract plain text and HTML body from email message."""
    body_text = ""
    body_html = ""

    if msg.is_multipart():
        for part in msg.walk():
            content_type = part.get_content_type()
            payload = part.get_payload(decode=True)

            if payload is None:
                continue

            if isinstance(payload, str):
                try:
                    charset = part.get_content_charset() or "utf-8"
                    payload = payload.encode(charset).decode(charset)
                except (UnicodeDecodeError, LookupError):
                    payload = payload.encode("utf-8", errors="replace").decode("utf-8", errors="replace")

            if content_type == "text/plain":
                body_text = payload
            elif content_type == "text/html":
                body_html = payload
    else:
        content_type = msg.get_content_type()
        payload = msg.get_payload(decode=True)

        if payload is None:
            payload = ""
        elif isinstance(payload, str):
            charset = msg.get_content_charset() or "utf-8"
            try:
                payload = payload.encode(charset).decode(charset)
            except (UnicodeDecodeError, LookupError):
                payload = payload.encode("utf-8", errors="replace").decode("utf-8", errors="replace")

        if content_type == "text/plain":
            body_text = payload
        elif content_type == "text/html":
            body_html = payload

    return body_text, body_html


def extract_attachments(msg) -> List[Tuple[str, str, bytes]]:
    """Extract attachments from email message.

    Returns list of (filename, content_type, data) tuples.
    """
    attachments = []

    if msg.is_multipart():
        for part in msg.walk():
            content_disposition = part.get("Content-Disposition", "")
            if "attachment" in content_disposition:
                filename = part.get_filename()
                if filename:
                    filename = decode_email_header(filename)
                else:
                    filename = "unnamed_attachment"

                content_type = part.get_content_type()
                data = part.get_payload(decode=True)

                if data:
                    attachments.append((filename, content_type, data))

    return attachments


def sync_emails():
    """Sync all emails since last sync."""
    import click

    print("Connecting to ProtonMail Bridge...")
    mail = connect()
    print("Connected.")

    # Get total message count using STATUS
    status, data = mail.status("INBOX", "(MESSAGES)")
    if status == "OK":
        # Response format: b'"INBOX" (MESSAGES 2450)'
        raw = data[0]
        # Find the number after MESSAGES
        import re
        match = re.search(rb'MESSAGES\s+(\d+)', raw)
        if match:
            total_messages = int(match.group(1))
            print(f"Total emails in INBOX: {total_messages}")
        else:
            total_messages = None
            print("Could not determine total email count")
    else:
        total_messages = None
        print("Could not determine total email count")

    # Select INBOX
    status, _ = mail.select("INBOX", readonly=True)
    if status != "OK":
        raise RuntimeError(f"Failed to select INBOX: {status}")

    # Get last synced UID
    last_uid = get_last_uid()
    print(f"Last synced UID: {last_uid}")

    # Search for all messages
    status, messages = mail.search(None, "ALL")
    if status != "OK":
        raise RuntimeError(f"Failed to search INBOX: {status}")

    if not messages[0]:
        print("No emails found in INBOX.")
        mail.logout()
        return

    all_ids = messages[0].split()

    # Filter to only new UIDs (messages with UID > last_uid)
    new_ids = [uid for uid in all_ids if int(uid) > last_uid]
    print(f"New emails to sync: {len(new_ids)}")

    if not new_ids:
        mail.logout()
        return

    highest_uid = last_uid

    def process_email(uid):
        """Fetch and store a single email. Returns (subject, num_attachments) or None on error."""
        try:
            status, msg_data = mail.fetch(uid, "(RFC822)")
            if status != "OK":
                return None

            if isinstance(msg_data[0], tuple):
                rfc822_data = msg_data[0][1]
            else:
                rfc822_data = msg_data[0]

            msg = email.message_from_bytes(rfc822_data)

            # Extract headers
            message_id = msg.get("Message-ID", "")
            if message_id:
                message_id = message_id.strip().strip("<>")

            subject = decode_email_header(msg.get("Subject", ""))
            from_addr = decode_email_header(msg.get("From", ""))
            to_addr = decode_email_header(msg.get("To", ""))

            date_str = msg.get("Date")
            date = None
            if date_str:
                try:
                    date = parsedate_to_datetime(date_str)
                except Exception:
                    date = None

            # Check if message is read
            flags = msg.get("Flags") or ""
            is_read = "SEEN" in flags.upper() if flags else False

            # Get raw headers
            raw_headers = msg.as_bytes().split(b"\r\n\r\n")[0].decode("utf-8", errors="replace")

            # Parse body
            body_text, body_html = parse_email_body(msg)

            # Insert email
            email_id = insert_email(
                message_id=message_id,
                subject=subject,
                from_addr=from_addr,
                to_addr=to_addr,
                date=date,
                body_text=body_text,
                body_html=body_html,
                raw_headers=raw_headers,
                is_read=is_read,
            )

            if email_id:
                # Extract and store attachments
                attachments = extract_attachments(msg)
                for filename, content_type, data in attachments:
                    insert_attachment(email_id, filename, content_type, data)
                return (subject, len(attachments))
            return None

        except Exception as e:
            return None

    # Use click progress bar
    with click.progressbar(
        new_ids,
        label="Syncing emails",
        show_eta=True,
        show_percent=True,
        item_show_func=lambda uid: f"UID {uid.decode() if isinstance(uid, bytes) else uid}"
    ) as bar:
        for uid in bar:
            result = process_email(uid)
            if result:
                highest_uid = max(highest_uid, int(uid))

    # Update last synced UID
    if highest_uid > last_uid:
        update_last_uid(highest_uid)
        print(f"Updated last UID to {highest_uid}")

    mail.logout()
    print("Sync complete.")
