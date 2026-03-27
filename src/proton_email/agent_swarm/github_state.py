"""GitHub state management utilities using the gh CLI."""

import os
import subprocess
import json
from pathlib import Path
from typing import Optional

STATE_DIR = Path(os.environ.get("MUCLAW_STATE_DIR", "/app/state"))
STATE_DIR.mkdir(parents=True, exist_ok=True)


def run_gh(args: list[str]) -> subprocess.CompletedProcess:
    """Run a gh CLI command and return the result."""
    return subprocess.run(
        ["gh"] + args,
        capture_output=True,
        text=True,
        check=True,
    )


def create_issue(title: str, body: str, labels: Optional[list[str]] = None) -> int:
    """Create a GitHub issue and return the issue number."""
    cmd = ["issue", "create", "--title", title, "--body", body]
    if labels:
        cmd.extend(["--label", ",".join(labels)])

    result = run_gh(cmd)
    # Output is URL like https://github.com/owner/repo/issues/123
    url = result.stdout.strip()
    issue_num = int(url.split("/")[-1])
    return issue_num


def get_issue(issue_num: int) -> dict:
    """Get issue details as a dict."""
    result = run_gh(["issue", "view", str(issue_num), "--json", "title,body,labels,state,assignees"])
    return json.loads(result.stdout)


def list_issues(filter_str: Optional[str] = None, state: str = "open") -> list[dict]:
    """List issues matching filter."""
    cmd = ["issue", "list", "--state", state, "--json", "number,title,labels,assignees"]
    if filter_str:
        cmd.extend(["--search", filter_str])

    result = run_gh(cmd)
    return json.loads(result.stdout)


def add_label(issue_num: int, label: str) -> None:
    """Add a label to an issue."""
    run_gh(["issue", "edit", str(issue_num), "--add-label", label])


def remove_label(issue_num: int, label: str) -> None:
    """Remove a label from an issue."""
    run_gh(["issue", "edit", str(issue_num), "--remove-label", label])


def assign_issue(issue_num: int, assignee: str) -> None:
    """Assign an issue to a user."""
    run_gh(["issue", "edit", str(issue_num), "--add-assignee", assignee])


def close_issue(issue_num: int) -> None:
    """Close a GitHub issue."""
    run_gh(["issue", "close", str(issue_num)])


def create_branch(issue_num: int, repo: Optional[str] = None) -> str:
    """Create a feature branch from issue number. Returns branch name."""
    issue = get_issue(issue_num)
    slug = issue["title"].lower().replace(" ", "-").replace("_", "-")
    slug = "".join(c if c.isalnum() or c == "-" else "" for c in slug)
    slug = slug[:50]  # Limit length
    branch_name = f"feature/{issue_num}-{slug}"

    subprocess.run(["git", "checkout", "-b", branch_name], check=True)
    return branch_name


def get_pr(pr_num: int) -> dict:
    """Get PR details as a dict."""
    result = run_gh(["pr", "view", str(pr_num), "--json", "title,body,state,merged,headRefName"])
    return json.loads(result.stdout)


def create_pr(
    title: str,
    body: str,
    head: Optional[str] = None,
    base: str = "main",
    draft: bool = False,
) -> int:
    """Create a PR and return the PR number."""
    cmd = ["pr", "create", "--title", title, "--body", body, "--base", base]
    if draft:
        cmd.append("--draft")
    if head:
        cmd.extend(["--head", head])

    result = run_gh(cmd)
    url = result.stdout.strip()
    pr_num = int(url.split("/")[-1])
    return pr_num


def merge_pr(pr_num: int, method: str = "squash") -> None:
    """Merge a PR. method is 'squash', 'merge', or 'rebase'."""
    run_gh(["pr", "merge", str(pr_num), "--admin", "--squash"])


def close_pr(pr_num: int) -> None:
    """Close a PR."""
    run_gh(["pr", "close", str(pr_num)])


def get_current_branch() -> str:
    """Get the current git branch name."""
    result = subprocess.run(
        ["git", "branch", "--show-current"],
        capture_output=True,
        text=True,
        check=True,
    )
    return result.stdout.strip()


def get_main_branch() -> str:
    """Get the main branch name (main or master)."""
    try:
        run_gh(["git", "rev-parse", "--verify", "origin/main"])
        return "main"
    except subprocess.CalledProcessError:
        return "master"


# State file helpers
def write_current_issue(issue_num: int) -> None:
    """Write the current issue number to state file."""
    (STATE_DIR / "current_issue.txt").write_text(str(issue_num))


def read_current_issue() -> Optional[int]:
    """Read the current issue number from state file."""
    f = STATE_DIR / "current_issue.txt"
    if f.exists():
        return int(f.read_text().strip())
    return None


def write_current_pr(pr_num: int) -> None:
    """Write the current PR number to state file."""
    (STATE_DIR / "current_pr.txt").write_text(str(pr_num))


def read_current_pr() -> Optional[int]:
    """Read the current PR number from state file."""
    f = STATE_DIR / "current_pr.txt"
    if f.exists():
        return int(f.read_text().strip())
    return None


def write_developer(identifier: str) -> None:
    """Write the developer identifier to state file."""
    (STATE_DIR / "developer.txt").write_text(identifier)


def read_developer() -> str:
    """Read the developer identifier from state file."""
    f = STATE_DIR / "developer.txt"
    if f.exists():
        return f.read_text().strip()
    return "@claude"


def write_qa_dev(identifier: str) -> None:
    """Write the QA dev identifier to state file."""
    (STATE_DIR / "qa_dev.txt").write_text(identifier)


def read_qa_dev() -> str:
    """Read the QA dev identifier from state file."""
    f = STATE_DIR / "qa_dev.txt"
    if f.exists():
        return f.read_text().strip()
    return "@qa-claude"
