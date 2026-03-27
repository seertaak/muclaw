"""
Database module for email storage with full-text search.
"""

import os
import sqlite3
from datetime import datetime
from pathlib import Path
from typing import Optional


# Allow DB path override via environment variable (useful for Docker)
DATA_DIR = Path(os.environ.get("PROTON_EMAIL_DATA_DIR", Path(__file__).parent.parent.parent / "data"))
DATA_DIR.mkdir(exist_ok=True)
DB_PATH = DATA_DIR / "emails.db"


def get_db() -> sqlite3.Connection:
    """Get a database connection with WAL mode enabled for concurrent access."""
    conn = sqlite3.connect(DB_PATH, timeout=30)
    conn.row_factory = sqlite3.Row
    # Enable WAL mode for concurrent reads/writes
    conn.execute("PRAGMA journal_mode=WAL")
    conn.execute("PRAGMA busy_timeout=30000")  # 30 second timeout for locks
    return conn


def init_db():
    """Initialize the database with schema and FTS5 support."""
    conn = get_db()
    cursor = conn.cursor()

    # Load schema from external SQL file
    schema_path = Path(__file__).parent.parent.parent / "schemas" / "email.sql"
    conn.executescript(schema_path.read_text())

    # Initialize sync_state if not exists
    cursor.execute("""
        INSERT OR IGNORE INTO sync_state (id, last_uid, last_sync)
        VALUES (1, 0, NULL)
    """)

    conn.commit()
    conn.close()


def get_last_uid() -> int:
    """Get the last synced UID."""
    conn = get_db()
    cursor = conn.cursor()
    cursor.execute("SELECT last_uid FROM sync_state WHERE id = 1")
    row = cursor.fetchone()
    conn.close()
    return row["last_uid"] if row else 0


def update_last_uid(uid: int):
    """Update the last synced UID."""
    conn = get_db()
    cursor = conn.cursor()
    cursor.execute(
        "UPDATE sync_state SET last_uid = ?, last_sync = CURRENT_TIMESTAMP WHERE id = 1",
        (uid,)
    )
    conn.commit()
    conn.close()


def insert_email(
    message_id: str,
    subject: str,
    from_addr: str,
    to_addr: str,
    date: Optional[datetime],
    body_text: str,
    body_html: str,
    raw_headers: str,
    is_read: bool = False,
) -> int:
    """Insert an email and return its ID."""
    conn = get_db()
    cursor = conn.cursor()
    try:
        cursor.execute(
            """
            INSERT INTO emails (
                message_id, subject, from_addr, to_addr, date,
                body_text, body_html, raw_headers, is_read, is_synced
            ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, 1)
            """,
            (message_id, subject, from_addr, to_addr, date, body_text, body_html, raw_headers, is_read),
        )
        conn.commit()
        email_id = cursor.lastrowid
    except sqlite3.IntegrityError:
        # Email already exists
        conn.rollback()
        cursor.execute("SELECT id FROM emails WHERE message_id = ?", (message_id,))
        row = cursor.fetchone()
        email_id = row["id"] if row else None
    finally:
        conn.close()
    return email_id


def insert_attachment(email_id: int, filename: str, content_type: str, data: bytes) -> int:
    """Insert an attachment and return its ID."""
    conn = get_db()
    cursor = conn.cursor()
    cursor.execute(
        """
        INSERT INTO attachments (email_id, filename, content_type, data, size)
        VALUES (?, ?, ?, ?, ?)
        """,
        (email_id, filename, content_type, data, len(data)),
    )
    conn.commit()
    attachment_id = cursor.lastrowid
    conn.close()
    return attachment_id


def get_recent_emails(limit: int = 5):
    """Get the most recent N emails, ordered by date descending."""
    conn = get_db()
    cursor = conn.cursor()
    cursor.execute(
        """
        SELECT id, message_id, subject, from_addr, to_addr, date, is_read
        FROM emails
        ORDER BY date DESC
        LIMIT ?
        """,
        (limit,),
    )
    rows = cursor.fetchall()
    conn.close()
    return [dict(row) for row in rows]


def search_emails(query: str, limit: int = 20):
    """Search emails using FTS5."""
    # Escape FTS5 special characters by wrapping in quotes
    # Escape any internal quotes by doubling them
    escaped = query.replace('"', '""')
    fts_query = f'"{escaped}"'

    conn = get_db()
    cursor = conn.cursor()
    try:
        cursor.execute(
            """
            SELECT e.id, e.message_id, e.subject, e.from_addr, e.date,
                   snippet(emails_fts, 1, '<mark>', '</mark>', '...', 32) as snippet
            FROM emails_fts
            JOIN emails e ON emails_fts.rowid = e.id
            WHERE emails_fts MATCH ?
            ORDER BY e.date DESC
            LIMIT ?
            """,
            (fts_query, limit),
        )
        rows = cursor.fetchall()
    finally:
        conn.close()
    return [dict(row) for row in rows]


def get_email_attachments(email_id: int):
    """Get all attachments for an email."""
    conn = get_db()
    cursor = conn.cursor()
    cursor.execute(
        """
        SELECT id, filename, content_type, size
        FROM attachments
        WHERE email_id = ?
        """,
        (email_id,),
    )
    rows = cursor.fetchall()
    conn.close()
    return [dict(row) for row in rows]


def get_attachment_data(attachment_id: int) -> Optional[bytes]:
    """Get attachment binary data."""
    conn = get_db()
    cursor = conn.cursor()
    cursor.execute("SELECT data FROM attachments WHERE id = ?", (attachment_id,))
    row = cursor.fetchone()
    conn.close()
    return row["data"] if row else None
