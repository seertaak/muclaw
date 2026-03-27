"""Orchestrator for the Dev Team Agent Swarm.

Spawns Claude Code CLI subprocesses for each role to implement
the virtual development team workflow.
"""

import os
import sys
import subprocess
import argparse
from pathlib import Path

from proton_email.agent_swarm.github_state import (
    read_current_issue,
    write_current_issue,
    read_current_pr,
    write_current_pr,
    write_developer,
    write_qa_dev,
)
from proton_email.telegram.bot import (
    send_hello,
    notify_issue_created,
    notify_development_started,
    notify_pr_created,
    notify_deployed,
    store_chat_id,
)

STATE_DIR = Path("/app/state")
STATE_DIR.mkdir(parents=True, exist_ok=True)


def spawn_claude(skill: str, extra_args: list[str] = None) -> subprocess.Popen:
    """Spawn a Claude Code CLI subprocess with a skill.

    Returns the Popen object for the subprocess.
    """
    cmd = ["claude", "--skill", skill]
    if extra_args:
        cmd.extend(extra_args)

    env = os.environ.copy()
    # Ensure we're in the right directory
    env["PWD"] = "/app"

    return subprocess.Popen(
        cmd,
        cwd="/app",
        env=env,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
    )


def cmd_start(initial_request: str, chat_id: str = None) -> None:
    """Start the swarm by launching the PM Assistant."""
    # Store chat ID if provided (for backward compatibility)
    if chat_id:
        store_chat_id(chat_id)

    print(f"[Orchestrator] Starting swarm for: {initial_request}")

    # Write the initial request to state
    (STATE_DIR / "initial_request.txt").write_text(initial_request)

    # Spawn PM Assistant
    print("[Orchestrator] Spawning PM Assistant...")
    proc = spawn_claude("pm-assistant", ["--request", initial_request])

    # Stream output
    for line in proc.stdout:
        print(f"[PM Assistant] {line}", end="")

    proc.wait()
    print(f"[Orchestrator] PM Assistant finished with code {proc.returncode}")


def cmd_status() -> None:
    """Show current status of all swarm components."""
    print("=== Swarm Status ===")

    # Check issue
    issue_num = read_current_issue()
    if issue_num:
        print(f"Current Issue: #{issue_num}")
    else:
        print("Current Issue: None")

    # Check PR
    pr_num = read_current_pr()
    if pr_num:
        print(f"Current PR: #{pr_num}")
    else:
        print("Current PR: None")

    # Show git branch
    result = subprocess.run(
        ["git", "branch", "--show-current"],
        capture_output=True,
        text=True,
    )
    branch = result.stdout.strip()
    if branch:
        print(f"Current Branch: {branch}")
    else:
        print("Current Branch: None (on detached HEAD or no repo)")

    # Show pending agents
    print("\nNo active agents currently running.")


def cmd_resume(issue_num: int = None) -> None:
    """Resume a stalled workflow for an issue."""
    if issue_num is None:
        issue_num = read_current_issue()

    if issue_num is None:
        print("[Orchestrator] No issue to resume. Use --issue to specify.")
        return

    print(f"[Orchestrator] Resuming issue #{issue_num}")
    spawn_claude("team-manager", ["--issue", str(issue_num)])


def cmd_invoke_team_manager(issue_num: int) -> None:
    """Invoke the team manager for a specific issue."""
    write_current_issue(issue_num)
    print(f"[Orchestrator] Invoking Team Manager for issue #{issue_num}")
    spawn_claude("team-manager", ["--issue", str(issue_num)])


def main():
    parser = argparse.ArgumentParser(description="Dev Team Agent Swarm Orchestrator")
    subparsers = parser.add_subparsers(dest="command", help="Commands")

    # start command
    start_parser = subparsers.add_parser("start", help="Start the swarm with an initial request")
    start_parser.add_argument("--request", "-r", required=True, help="Initial request or issue description")
    start_parser.add_argument("--chat-id", help="Telegram chat ID for updates")

    # status command
    subparsers.add_parser("status", help="Show current swarm status")

    # resume command
    resume_parser = subparsers.add_parser("resume", help="Resume a stalled workflow")
    resume_parser.add_argument("--issue", "-i", type=int, help="Issue number to resume")

    # invoke-team-manager command (internal use)
    invoke_parser = subparsers.add_parser("invoke-team-manager", help="Invoke team manager for an issue")
    invoke_parser.add_argument("--issue", "-i", type=int, required=True, help="Issue number")

    # hello command (test Telegram)
    subparsers.add_parser("hello", help="Send a hello message via Telegram to verify setup")

    args = parser.parse_args()

    if args.command == "start":
        cmd_start(args.request, args.chat_id)
    elif args.command == "status":
        cmd_status()
    elif args.command == "resume":
        cmd_resume(args.issue)
    elif args.command == "invoke-team-manager":
        cmd_invoke_team_manager(args.issue)
    elif args.command == "hello":
        print("[Orchestrator] Sending hello via Telegram...")
        send_hello()
    else:
        parser.print_help()


if __name__ == "__main__":
    main()
