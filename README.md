# MuClaw - Michelle's Little Claw

A personal AI assistant built to specification. Perfect.

## Project Structure

```
muclaw/
├── src/
│   └── proton_email/        # Main package
│       ├── __init__.py
│       ├── __main__.py      # CLI entry point
│       ├── cli.py           # CLI commands
│       ├── db.py            # Database schema and operations
│       └── email_client.py  # Client interface
├── data/                    # SQLite database and data
├── pyproject.toml           # Package configuration
├── uv.lock                  # Dependency lock file
├── Dockerfile
└── docker-compose.yml
```

## Prerequisites

- Python 3.10+
- Docker and Docker Compose (for containerized deployment)
- API keys as needed for your configured clients

## Local Development Setup

1. Install dependencies with uv:

```bash
uv sync
```

2. Configure environment variables in `.env`:

```bash
# See Environment Variables section below
```

3. Run the CLI:

```bash
uv run proton-email --help
```

## Docker Deployment

This project includes Docker support for easy deployment. The Dockerfile is pre-configured with:
- Python 3.12 runtime
- Claude Code CLI for AI operations
- GitHub CLI for repository integration
- Cron for automatic sync every minute

### Building the Docker Image

Build the image using Docker Compose:

```bash
docker compose build
```

Or build directly with Docker:

```bash
docker build -t muclaw .
```

### Running the Container

#### Option 1: Using Docker Compose (Recommended)

1. Create a `.env` file with your secrets:

```bash
# Required for Telegram integration
TELEGRAM_BOT_API_TOKEN=your_telegram_bot_token
TELEGRAM_USER_ID=your_telegram_user_id

# Required for GitHub operations
GITHUB_USER_EMAIL=your_github_email
GITHUB_USER_NAME=your_github_username
GITHUB_SSH_KEY=/run/secrets/github_ssh_key  # Path to SSH key in container

# Optional: For AI features
MINIMAX_API_KEY=your_minimax_api_key
```

2. Place your `.env` file in your home directory (`~/.env`)

3. Start the container:

```bash
docker compose up -d
```

#### Option 2: Using Docker Secrets

For production deployments, use Docker secrets for sensitive data:

1. Create Docker secret files:

```bash
# Create the secrets directory
mkdir -p /run/secrets

# Create the secrets file
echo "TELEGRAM_BOT_API_TOKEN=your_telegram_bot_token
TELEGRAM_USER_ID=your_telegram_user_id
GITHUB_USER_EMAIL=your_github_email
GITHUB_USER_NAME=your_github_username" > /run/secrets/proton_email.env
```

2. Run with secrets:

```bash
docker run -d \
  --name muclaw \
  --restart unless-stopped \
  -v $(pwd)/data:/app/data \
  -v muclaw_state:/app/state \
  --env-file /run/secrets/proton_email.env \
  muclaw
```

### Volume Mounts for Data Persistence

The following volumes are used for data persistence:

| Volume | Container Path | Description |
|--------|---------------|-------------|
| `./data` (host) | `/app/data` | SQLite database and attachments |
| `muclaw_state` (named) | `/app/state` | Application state and worktrees |
| `~/.env` or secrets | `/run/secrets/proton_email.env` | Environment variables (read-only) |

### Environment Variables

| Variable | Required | Description |
|----------|----------|-------------|
| `TELEGRAM_BOT_API_TOKEN` | Yes | Telegram bot API token for notifications |
| `TELEGRAM_USER_ID` | Yes | Your Telegram user ID for authorization |
| `GITHUB_USER_EMAIL` | No | GitHub email for git operations |
| `GITHUB_USER_NAME` | No | GitHub username for git operations |
| `GITHUB_SSH_KEY` | No | Path to SSH key in container for GitHub access |
| `MINIMAX_API_KEY` | No | MiniMax API key for AI features |

### Automatic Sync

When the container starts without arguments, cron automatically runs email sync every minute. Check logs with:

```bash
docker compose logs -f
# or for the sync log specifically
docker exec muclaw tail -f /var/log/proton_email_sync.log
```

### Running Commands Manually

Execute commands inside the container:

```bash
# Sync emails
docker compose exec proton-email proton-email sync

# List recent records
docker compose exec proton-email proton-email list

# Search emails
docker compose exec proton-email proton-email search "query"

# Open an interactive shell
docker compose exec proton-email bash
```

## Database

The SQLite database (`data/emails.db`) stores assistant data with:
- **emails**: Core data records
- **attachments**: Binary blobs for data attachments
- **emails_fts**: Full-text search index (FTS5)
- **sync_state**: Tracks incremental sync state

SQLite is configured with WAL mode for safe concurrent access.

## Environment Variables

| Variable | Default | Description |
|----------|---------|-------------|
| `PROTON_EMAIL_DATA_DIR` | `./data` | Directory for database |
| `PROTON_EMAIL_ENV_PATH` | `./.env` | Path to .env file |

## Usage

```bash
uv run proton-email sync      # Sync data
uv run proton-email list [n]  # Show last N records (default: 5)
uv run proton-email search <query>  # Full-text search
uv run proton-email show <id>  # Show full details
```
