"""Orchestrator for the Dev Team Agent Swarm.

Spawns AI agents using the Anthropic SDK for each role to implement
the virtual development team workflow.
"""

import os
import sys
import subprocess
import argparse
from pathlib import Path

import anthropic

ENV_FILE = "/run/secrets/proton_email.env"


def _load_env():
    """Load env vars from secrets file if not already set."""
    if os.environ.get("ANTHROPIC_API_KEY"):
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


_load_env()

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

SKILLS_DIR = Path("/app/.claude/skills")

# GitHub tools available to agents
GITHUB_TOOLS = [
    {
        "name": "run_gh",
        "description": "Run a GitHub CLI command. Use this to create issues, PRs, etc.",
        "input_schema": {
            "type": "object",
            "properties": {
                "args": {"type": "string", "description": "The gh command args (e.g., 'issue create --title Foo --body Bar')"},
            },
            "required": ["args"],
        },
    },
    {
        "name": "write_file",
        "description": "Write content to a file",
        "input_schema": {
            "type": "object",
            "properties": {
                "path": {"type": "string", "description": "File path (relative to /app)"},
                "content": {"type": "string", "description": "Content to write"},
            },
            "required": ["path", "content"],
        },
    },
    {
        "name": "read_file",
        "description": "Read content from a file",
        "input_schema": {
            "type": "object",
            "properties": {
                "path": {"type": "string", "description": "File path (relative to /app)"},
            },
            "required": ["path"],
        },
    },
    {
        "name": "run_command",
        "description": "Run a shell command",
        "input_schema": {
            "type": "object",
            "properties": {
                "command": {"type": "string", "description": "Command to run"},
                "cwd": {"type": "string", "description": "Working directory (default /app)"},
            },
            "required": ["command"],
        },
    },
]


def get_client() -> anthropic.Anthropic:
    """Get the Anthropic client configured for Minimax."""
    return anthropic.Anthropic(
        base_url="https://api.minimax.io/anthropic"
    )


def load_skill(skill_name: str) -> str:
    """Load a skill file as a system prompt."""
    skill_path = SKILLS_DIR / f"{skill_name}.md"
    return skill_path.read_text()


def run_gh_command(args: str) -> str:
    """Execute a gh command and return output."""
    result = subprocess.run(
        ["gh"] + args.split(),
        capture_output=True,
        text=True,
        cwd="/app",
    )
    return result.stdout + result.stderr


def run_shell_command(command: str, cwd: str = "/app") -> str:
    """Execute a shell command and return output."""
    result = subprocess.run(
        command,
        shell=True,
        capture_output=True,
        text=True,
        cwd=cwd,
    )
    return result.stdout + result.stderr


def spawn_agent(skill_name: str, user_message: str, context: dict = None) -> str:
    """Spawn an agent with the given skill and get its response.

    Returns the agent's final response text.
    """
    client = get_client()
    system_prompt = load_skill(skill_name)

    if context:
        context_str = "\n".join([f"- {k}: {v}" for k, v in context.items()])
        system_prompt += f"\n\n## Context\n{context_str}"

    messages = [{"role": "user", "content": user_message}]
    max_turns = 10
    turn = 0

    while turn < max_turns:
        turn += 1

        response = client.messages.create(
            model="MiniMax-M2.7",
            max_tokens=4096,
            system=system_prompt,
            messages=messages,
            tools=GITHUB_TOOLS,
        )

        # Add assistant response to messages
        messages.append({"role": "assistant", "content": response.content})

        # Check if we have stop reason (no more tool calls needed)
        if response.stop_reason == "end_turn":
            # Extract text content
            text = ""
            for block in response.content:
                if hasattr(block, 'text'):
                    text += block.text
            return text

        # Process tool uses
        for block in response.content:
            if hasattr(block, 'type') and block.type == "tool_use":
                tool_name = block.name
                tool_input = block.input

                if tool_name == "run_gh":
                    result = run_gh_command(tool_input["args"])
                elif tool_name == "run_command":
                    result = run_shell_command(
                        tool_input["command"],
                        tool_input.get("cwd", "/app")
                    )
                elif tool_name == "write_file":
                    path = Path(tool_input["path"])
                    if not path.is_absolute():
                        path = Path("/app") / path
                    path.parent.mkdir(parents=True, exist_ok=True)
                    path.write_text(tool_input["content"])
                    result = f"Written to {path}"
                elif tool_name == "read_file":
                    path = Path(tool_input["path"])
                    if not path.is_absolute():
                        path = Path("/app") / path
                    result = path.read_text()
                else:
                    result = f"Unknown tool: {tool_name}"

                # Add tool result to messages
                messages.append({
                    "role": "user",
                    "content": [{
                        "type": "tool_result",
                        "tool_use_id": block.id,
                        "content": result,
                    }]
                })

    return "Max turns reached"


def cmd_start(initial_request: str, chat_id: str = None) -> None:
    """Start the swarm by launching the PM Assistant."""
    if chat_id:
        store_chat_id(chat_id)

    print(f"[Orchestrator] Starting swarm for: {initial_request}")

    # Write the initial request to state
    (STATE_DIR / "initial_request.txt").write_text(initial_request)

    # Spawn PM Assistant
    print("[Orchestrator] Spawning PM Assistant...")
    response = spawn_agent(
        "pm-assistant",
        initial_request,
        context={"muclaw_dir": "/app", "state_dir": str(STATE_DIR)}
    )
    print(f"[PM Assistant] {response}")


def cmd_status() -> None:
    """Show current status of all swarm components."""
    print("=== Swarm Status ===")

    issue_num = read_current_issue()
    if issue_num:
        print(f"Current Issue: #{issue_num}")
    else:
        print("Current Issue: None")

    pr_num = read_current_pr()
    if pr_num:
        print(f"Current PR: #{pr_num}")
    else:
        print("Current PR: None")

    result = subprocess.run(
        ["git", "branch", "--show-current"],
        capture_output=True,
        text=True,
    )
    branch = result.stdout.strip()
    if branch:
        print(f"Current Branch: {branch}")
    else:
        print("Current Branch: None")

    print("\nNo active agents currently running.")


def cmd_resume(issue_num: int = None) -> None:
    """Resume a stalled workflow for an issue."""
    if issue_num is None:
        issue_num = read_current_issue()

    if issue_num is None:
        print("[Orchestrator] No issue to resume. Use --issue to specify.")
        return

    print(f"[Orchestrator] Resuming issue #{issue_num}")
    response = spawn_agent(
        "team-manager",
        f"Resume working on issue #{issue_num}",
        context={"issue_num": str(issue_num), "muclaw_dir": "/app"}
    )
    print(f"[Team Manager] {response}")


def cmd_invoke_team_manager(issue_num: int) -> None:
    """Invoke the team manager for a specific issue."""
    write_current_issue(issue_num)
    print(f"[Orchestrator] Invoking Team Manager for issue #{issue_num}")
    response = spawn_agent(
        "team-manager",
        f"Process issue #{issue_num}",
        context={"issue_num": str(issue_num), "muclaw_dir": "/app"}
    )
    print(f"[Team Manager] {response}")


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
