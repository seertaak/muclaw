"""
CLI module for email client.
"""

import click
from tabulate import tabulate

from .db import init_db, get_recent_emails, search_emails


@click.group()
def cli():
    """ProtonMail Bridge email client."""
    init_db()


@cli.command()
def sync():
    """Sync emails from ProtonMail Bridge."""
    from .email_client import sync_emails
    sync_emails()


@cli.command()
@click.argument("count", default=5, required=False)
def list(count):
    """Show the last N emails (default: 5)."""
    emails = get_recent_emails(count)

    if not emails:
        click.echo("No emails in database. Run 'sync' first.")
        return

    headers = ["Date", "From", "Subject"]
    rows = []
    for e in emails:
        subject = e["subject"] or "(No subject)"
        if len(subject) > 50:
            subject = subject[:47] + "..."
        from_addr = e["from_addr"] or "Unknown"
        if len(from_addr) > 30:
            from_addr = from_addr[:27] + "..."
        date = e["date"] or "Unknown"
        if isinstance(date, str):
            # Truncate timestamp for display
            if len(date) > 19:
                date = date[:19]
        rows.append([date, from_addr, subject])

    click.echo(tabulate(rows, headers=headers, tablefmt="simple"))


@cli.command()
@click.argument("query")
@click.option("--limit", "-n", default=20, help="Maximum number of results")
def search(query, limit):
    """Search emails using full-text search."""
    results = search_emails(query, limit)

    if not results:
        click.echo(f"No results found for: {query}")
        return

    click.echo(f"Found {len(results)} results for: {query}\n")

    headers = ["ID", "Subject", "From", "Date"]
    rows = []
    for r in results:
        subject = r["subject"] or "(No subject)"
        if len(subject) > 40:
            subject = subject[:37] + "..."
        from_addr = r["from_addr"] or "Unknown"
        if len(from_addr) > 25:
            from_addr = from_addr[:22] + "..."
        date = r["date"] or "Unknown"
        if isinstance(date, str) and len(date) > 19:
            date = date[:19]
        rows.append([r["id"], subject, from_addr, date])

    click.echo(tabulate(rows, headers=headers, tablefmt="simple"))
    click.echo(f"\nUse 'show <id>' to view full email details.")


@cli.command()
@click.argument("email_id", type=int)
def show(email_id):
    """Show full details of an email by ID."""
    from .db import get_db, get_email_attachments

    conn = get_db()
    cursor = conn.cursor()
    cursor.execute(
        """
        SELECT id, message_id, subject, from_addr, to_addr, date, body_text
        FROM emails WHERE id = ?
        """,
        (email_id,),
    )
    row = cursor.fetchone()
    conn.close()

    if not row:
        click.echo(f"Email with ID {email_id} not found.")
        return

    email = dict(row)

    click.echo(f"{'='*70}")
    click.echo(f"ID: {email['id']}")
    click.echo(f"From: {email['from_addr']}")
    click.echo(f"To: {email['to_addr']}")
    click.echo(f"Date: {email['date']}")
    click.echo(f"Subject: {email['subject']}")
    click.echo(f"{'='*70}")
    click.echo()

    if email["body_text"]:
        click.echo(email["body_text"][:2000])
        if len(email["body_text"]) > 2000:
            click.echo(f"\n... (truncated, {len(email['body_text'])} total chars)")
    else:
        click.echo("(No text body)")

    # Show attachments
    attachments = get_email_attachments(email_id)
    if attachments:
        click.echo(f"\n{'='*70}")
        click.echo(f"Attachments ({len(attachments)}):")
        for att in attachments:
            click.echo(f"  - {att['filename']} ({att['content_type']}, {att['size']} bytes)")
