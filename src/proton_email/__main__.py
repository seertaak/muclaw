#!/usr/bin/env python3
"""
ProtonMail Bridge email client with SQLite storage and full-text search.

Usage:
    python -m proton_email sync      # Sync emails from Bridge
    python -m proton_email list [n]  # Show last N emails (default: 5)
    python -m proton_email search <query>  # Full-text search
    python -m proton_email show <id>  # Show full email details
"""

from .cli import cli

if __name__ == "__main__":
    cli()
