"""Agent Swarm - Virtual development team orchestration."""

from proton_email.agent_swarm.orchestrator import main
from proton_email.agent_swarm.github_state import (
    create_issue,
    get_issue,
    list_issues,
    create_branch,
    create_pr,
    merge_pr,
    close_issue,
    read_current_issue,
    write_current_issue,
    read_current_pr,
    write_current_pr,
)

__all__ = [
    "main",
    "create_issue",
    "get_issue",
    "list_issues",
    "create_branch",
    "create_pr",
    "merge_pr",
    "close_issue",
    "read_current_issue",
    "write_current_issue",
    "read_current_pr",
    "write_current_pr",
]
