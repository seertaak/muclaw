# Dev Team Agent Swarm - Implementation Plan

## Context

You want to replicate a virtual development team workflow (Product Manager → Team Manager → Developer → QA Dev) using Claude AI agents as Claude Code CLI subprocesses. The agents communicate via GitHub (issues, branches, PRs) and broadcast status via Telegram. The system develops the muclaw codebase itself.

## Architecture Overview

```
Human PM (Claude Code CLI)
    │
    ▼
┌─────────────────────────────────────────────────────────────────────┐
│                     Orchestrator (Python CLI)                        │
│  - Spawns Claude Code CLI subprocesses for each role                 │
│  - Coordinates workflow state via GitHub                            │
│  - Broadcasts status to Telegram Bot                               │
└─────────────────────────────────────────────────────────────────────┘
    │
    ├──▶ PM Assistant (Claude Code + skill)
    │       - Q/A with PM → draft issue → sign-off → create issue
    │
    ▼
┌─────────────────────────────────────────────────────────────────────┐
│                     Team Manager (Claude Code + skill)              │
│  - Assigns the single developer to the issue                       │
│  - Spawns Developer (QA Dev is spawned by Developer later)          │
└─────────────────────────────────────────────────────────────────────┘
    │
    ▼
┌─────────────────────────────────────────────────────────────────────┐
│                     Developer (Claude Code + skill)                 │
│  - Creates branch → implements → tests → deploys dev → PR          │
│  - Spawns QA Dev to review the PR                                   │
└─────────────────────────────────────────────────────────────────────┘
    │
    ▼
┌─────────────────────────────────────────────────────────────────────┐
│                     QA Dev (Claude Code + skill)                    │
│  - Reviews PR → accepts/rejects                                     │
│  - If accept: Developer merges → deploys prod → closes issue       │
└─────────────────────────────────────────────────────────────────────┘
```

**Note on future parallelization**: The Team Manager will eventually monitor the issue board on a cron loop and assign multiple developers in parallel. For now, single-threaded development.

## Skills to Create

Each role gets a skill definition in `.claude/skills/`:

| Skill | File | Purpose |
|-------|------|---------|
| pm-assistant | `.claude/skills/pm-assistant.md` | Q/A clarification, issue drafting, GitHub issue creation |
| team-manager | `.claude/skills/team-manager.md` | Issue prioritization, developer assignment, spawning agents |
| developer | `.claude/skills/developer.md` | Development loop (branch, implement, test, PR) |
| qa-dev | `.claude/skills/qa-dev.md` | PR review, acceptance/rejection |

## Critical Files to Create

### 1. `.claude/skills/pm-assistant.md`
- Role: Product Manager Assistant
- Responsibilities:
  - Initiate Q/A prompts to clarify issue/feature
  - Draft GitHub issue with description and A/C
  - Request PM sign-off
  - Create issue via `gh issue create`
  - Invoke Team Manager after sign-off

### 2. `.claude/skills/team-manager.md`
- Role: Development Team Manager
- Responsibilities:
  - Receives notification that a new issue was created by PM Assistant
  - Assigns the single developer to the issue (no prioritization needed)
  - Spawns Developer agent with issue context
  - (Future: cron-based monitoring of issue board for parallelization)

### 3. `.claude/skills/developer.md`
- Role: Developer
- Responsibilities:
  - Checkout/create feature branch from issue
  - Implement changes iteratively
  - Write unit/integration tests
  - Create PR via `gh pr create`
  - Spawn QA Dev to review the PR
  - Handle review feedback (if rejected)
  - On QA acceptance: squash-merge PR → deploy (single Hetzner instance) → close issue

### 4. `.claude/skills/qa-dev.md`
- Role: QA Developer
- Responsibilities:
  - Review PR critically (code quality, test coverage)
  - Suggest changes or improvements
  - Accept/reject PR via `gh pr merge` or request changes
  - On accept: notify Developer to finalize

### 5. `src/muclaw/orchestrator.py` (new)
- Main entry point for the swarm
- Commands:
  - `muclaw swarm start --issue "<description>"` - Start PM Assistant
  - `muclaw swarm status` - Show current issue/branch/PR status
  - `muclaw swarm resume <issue-number>` - Resume stalled workflow
- Spawns Claude Code CLI as subprocess with `--skill` flag
- Manages workflow state (issue → branch → PR → merge)

### 6. `src/muclaw/telegram_bot.py` (new)
- Telegram Bot integration (via Docker secret at `/run/secrets/telegram_token`)
- Broadcasts status updates to PM:
  - "Issue #123 created"
  - "Developer started working on issue #123"
  - "PR #45 created for review"
  - "PR #45 merged and deployed to prod"
- After deployment, sends PM a message with:
  - Summary of changes
  - How to test the new functionality
  - Commands to roll back if needed (e.g., `/rollback issue/123` or `/rollback pr/45`)
- Rollback command:
  - `git revert` the merged commit
  - Restart affected services/cron if needed
  - Notify PM of rollback status

### 7. `src/muclaw/github_state.py` (new)
- GitHub state management utilities
- Functions:
  - `create_issue(title, body, labels)` → `gh issue create`
  - `get_issue(issue_num)` → `gh issue view`
  - `list_issues(filter)` → `gh issue list`
  - `create_branch(issue_num)` → `git checkout -b`
  - `create_pr(body)` → `gh pr create`
  - `merge_pr(pr_num)` → `gh pr merge`
  - `add_label/remove_label`

## Workflow Sequence

1. **PM starts**: `muclaw swarm start --issue "Add dark mode support"`
2. **PM Assistant**: Q/A loop → drafts issue → PM approves → `gh issue create`
3. **PM Assistant**: Invokes Team Manager (via Telegram signal or file)
4. **Team Manager**: Assigns developer to issue → spawns Developer agent
5. **Developer**: `git checkout -b feature/123-dark-mode` → implement → test → `gh pr create` → spawns QA Dev
6. **QA Dev**: `gh pr review 45` → reviews → accepts
7. **Developer**: squash-merge PR → deploy to prod (Hetzner instance) → `gh issue close 123`
8. **Telegram Bot**: Notifies PM of deployment with testing instructions and rollback command
9. **PM tests**: If satisfied, done. If not, PM sends `/rollback` command → git revert → restart services

## Docker Changes

Update `Dockerfile` to include:
- Claude Code CLI (`npm install -g @anthropic-ai/claude-code`)
- `gh` CLI for GitHub integration
- Telegram Bot dependencies (`python-telegram-bot`)
- Docker secret mount for Telegram token at `/run/secrets/telegram_token`
- Entry point modified to support `muclaw swarm` commands

## Verification

1. **Unit test the orchestrator**:
   ```bash
   uv run muclaw swarm start --issue "Test issue"
   ```
   Should launch PM Assistant skill

2. **Test GitHub integration**:
   ```bash
   gh issue create --title "Test" --body "Test"
   gh issue list
   ```
   Verify `gh` CLI is authenticated

3. **Test Telegram bot**:
   - Send test message via Bot API
   - Verify bot broadcasts status

4. **End-to-end test**:
   - Create issue via swarm
   - Verify branch created
   - Verify PR opened
   - Verify merge workflow completes
