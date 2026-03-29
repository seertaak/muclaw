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

### Building the Docker Image

```bash
# Build the image locally
docker build -t muclaw .

# Or use Docker Compose (which builds automatically)
docker compose build
```

### Running with Docker Compose

1. Ensure your `.env` file is in your home directory (`~/.env`)

2. Build and run:

```bash
docker compose up -d
```

3. View logs:

```bash
docker compose logs -f
```

### Environment Variables

Configure the following environment variables in your `.env` file:

| Variable | Description |
|----------|-------------|
| `TELEGRAM_BOT_API_TOKEN` | Telegram bot token for notifications |
| `TELEGRAM_USER_ID` | Your Telegram user ID for authorization |
| `GITHUB_USER_EMAIL` | GitHub user email for git operations |
| `GITHUB_USER_NAME` | GitHub username for git operations |
| `GITHUB_SSH_KEY` | Path to SSH private key for GitHub access |
| `GITHUB_PERSONAL_ACCESS_TOKEN` | GitHub PAT for API access |
| `MINIMAX_API_KEY` | MiniMax API key for AI features |

### Docker Secrets

For production deployments, use Docker secrets instead of plain text `.env` files:

```bash
# Create the secrets file
echo "TELEGRAM_BOT_API_TOKEN=your_token_here" | docker secret create proton_email_secrets.env -
echo "TELEGRAM_USER_ID=123456789" | docker secret create proton_email_secrets.env -
# ... add other secrets

# Update docker-compose.yml to use secrets (Swarm mode)
services:
  proton-email:
    secrets:
      - proton_email_secrets
    # ...

secrets:
  proton_email_secrets:
    external: true
```

The entrypoint script loads secrets from `/run/secrets/proton_email.env` or `/run/secrets/secrets.env`.

### Volume Mounts for Data Persistence

The container uses the following volumes:

| Volume | Mount Point | Purpose |
|--------|-------------|---------|
| `./data` | `/app/data` | SQLite database and attachments |
| `muclaw_state` | `/app/state` | Application state and worktrees |
| `~/.env` | `/run/secrets/proton_email.env` | Secrets (read-only) |

### Automatic Sync

When running without arguments, the container starts cron which automatically syncs data every minute. To run manual commands instead:

```bash
# Run a single sync
docker compose run --rm proton-email proton-email sync

# List recent records
docker compose run --rm proton-email proton-email list

# Search emails
docker compose run --rm proton-email proton-email search "query"

# Open an interactive shell
docker compose run --rm proton-email bash
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
