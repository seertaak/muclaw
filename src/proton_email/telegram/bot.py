"""Telegram bot for broadcasting status updates to the PM."""

import os
import subprocess
from typing import Optional

TELEGRAM_STATE_DIR = os.environ.get("MUCLAW_STATE_DIR", "/app/state")
ENV_FILE = "/run/secrets/proton_email.env"


def _load_env():
    """Load env vars from secrets file if not already set."""
    if os.environ.get("TELEGRAM_BOT_API_TOKEN"):
        return  # Already loaded
    try:
        with open(ENV_FILE) as f:
            for line in f:
                line = line.strip()
                if line and not line.startswith("#") and "=" in line:
                    key, value = line.split("=", 1)
                    os.environ[key] = value
    except (FileNotFoundError, IOError):
        pass


def get_telegram_token() -> Optional[str]:
    """Get Telegram bot token from environment variable."""
    _load_env()
    return os.environ.get("TELEGRAM_BOT_API_TOKEN")


def get_default_chat_id() -> Optional[str]:
    """Get the default Telegram user ID from environment variable."""
    _load_env()
    return os.environ.get("TELEGRAM_USER_ID")


def send_message(text: str, chat_id: str = None) -> bool:
    """Send a message via Telegram bot using curl."""
    token = get_telegram_token()
    if not token:
        print("[Telegram] Token not found (TELEGRAM_BOT_API_TOKEN env var not set)")
        return False

    if not chat_id:
        chat_id = get_default_chat_id()
    if not chat_id:
        print("[Telegram] Chat ID not found (TELEGRAM_USER_ID env var not set)")
        return False

    try:
        result = subprocess.run(
            [
                "curl",
                "-s",
                "-X",
                "POST",
                f"https://api.telegram.org/bot{token}/sendMessage",
                "-d",
                f"chat_id={chat_id}",
                "-d",
                f"text={text}",
                "-d",
                "parse_mode=Markdown",
            ],
            capture_output=True,
            text=True,
            timeout=10,
        )
        if result.returncode == 0:
            print(f"[Telegram] Sent: {text[:50]}...")
            return True
        else:
            print(f"[Telegram] Error: {result.stderr}")
            return False
    except Exception as e:
        print(f"[Telegram] Exception: {e}")
        return False


def notify_issue_created(issue_num: int, title: str) -> None:
    """Notify PM that an issue was created."""
    text = (
        f"✅ *Issue #{issue_num} created*\n\n"
        f"_{title}_\n\n"
        f"Team Manager will assign a developer shortly."
    )
    send_message(text)


def notify_development_started(issue_num: int, developer: str) -> None:
    """Notify PM that development has started."""
    text = (
        f"🚀 *Development started* on issue #{issue_num}\n\n"
        f"Developer: {developer}\n\n"
        f"I'll notify you when the PR is ready for review."
    )
    send_message(text)


def notify_pr_created(pr_num: int, issue_num: int, title: str) -> None:
    """Notify PM that a PR was created for review."""
    text = (
        f"🔍 *PR #{pr_num} created* for issue #{issue_num}\n\n"
        f"_{title}_\n\n"
        f"QA Dev is reviewing..."
    )
    send_message(text)


def notify_pr_accepted(pr_num: int) -> None:
    """Notify PM that QA accepted the PR."""
    text = (
        f"✅ *QA approved* PR #{pr_num}\n\n"
        f"Merging and deploying..."
    )
    send_message(text)


def notify_pr_rejected(pr_num: int, feedback: str) -> None:
    """Notify PM that QA rejected the PR."""
    text = (
        f"❌ *QA requested changes* on PR #{pr_num}\n\n"
        f"Feedback: {feedback}\n\n"
        f"Developer will address and resubmit."
    )
    send_message(text)


def notify_deployed(issue_num: int, pr_num: int, title: str, body: str) -> None:
    """Notify PM of successful deployment with testing instructions."""
    text = (
        f"🎉 *Deployed!* Issue #{issue_num}, PR #{pr_num}\n\n"
        f"*{title}*\n\n"
        f"--- Testing Instructions ---\n"
        f"{body}\n\n"
        f"--- Rollback ---\n"
        f"`/rollback pr/{pr_num}` or `/rollback issue/{issue_num}`"
    )
    send_message(text)


def send_hello() -> bool:
    """Send a hello message to the PM to verify Telegram is working."""
    text = (
        "👋 *Hello from MuClaw!*\n\n"
        "I'm online and ready.\n"
        "Use `muclaw swarm start --request \"your idea\"` to start a new feature."
    )
    return send_message(text)


def notify_rollback(pr_num: int, issue_num: int, success: bool) -> None:
    """Notify PM of rollback status."""
    if success:
        text = (
            f"↩️ *Rollback complete*\n\n"
            f"PR #{pr_num} reverted. Issue #{issue_num} re-opened.\n"
            f"Services restarted."
        )
    else:
        text = (
            f"⚠️ *Rollback failed*\n\n"
            f"PR #{pr_num} / Issue #{issue_num}\n"
            f"Please check logs and handle manually."
        )
    send_message(text)


def get_stored_chat_id() -> Optional[str]:
    """Get the stored Telegram chat ID for the PM."""
    f = f"{TELEGRAM_STATE_DIR}/telegram_chat_id.txt"
    try:
        return open(f).read().strip()
    except (FileNotFoundError, IOError):
        return None


def store_chat_id(chat_id: str) -> None:
    """Store the Telegram chat ID for the PM."""
    os.makedirs(TELEGRAM_STATE_DIR, exist_ok=True)
    open(f"{TELEGRAM_STATE_DIR}/telegram_chat_id.txt", "w").write(chat_id)
