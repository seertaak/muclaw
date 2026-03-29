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

This project includes Docker support for containerized deployment. The container runs a cron job that syncs data every minute automatically.

### Prerequisites

- Docker 20.10+ and Docker Compose v2+
- A `.env` file with your configuration (see [Environment Variables](#environment-variables))

### Building the Docker Image

Build the image using Docker Compose:

```bash
docker compose build
```

Or build directly with `docker build`:

```bash
docker build -t muclaw .
```

### Running the Container

#### Option 1: Using Docker Compose (Recommended)

1. Create a `.env` file in your project directory with your secrets:

```bash
# Required for Telegram notifications
TELEGRAM_BOT_API_TOKEN=your_telegram_bot_token
TELEGRAM_USER_ID=your_telegram_user_id

# Optional: GitHub integration
GITHUB_USER_EMAIL=your_email@example.com
GITHUB_USER_NAME=your_github_username
GITHUB_SSH_KEY=/root/.ssh/id_ed25519

# Optional: AI API
MINIMAX_API_KEY=your_minimax_api_key
```

2. Start the container:

```bash
docker compose up -d
```

#### Option 2: Using Docker Secrets

For swarm deployments, you can use Docker secrets:

1. Create a secrets file:

```bash
echo "your_secret_value" | docker secret create proton_email_telegram_token -
```

2. Update your `docker-compose.yml` to use secrets:

```yaml
secrets:
  proton_email_telegram_token:
    external: true
  proton_email_telegram_user_id:
    external: true

services:
  proton-email:
    secrets:
      - proton_email_telegram_token
      - proton_email_telegram_user_id
    environment:
      TELEGRAM_BOT_API_TOKEN_FILE: /run/secrets/proton_email_telegram_token
      TELEGRAM_USER_ID_FILE: /run/secrets/proton_email_telegram_user_id
```

The entrypoint script loads secrets from `/run/secrets/proton_email.env` or `/run/secrets/proton_email_<key>.env` format.

### Environment Variables

| Variable | Description | Required |
|----------|-------------|----------|
| `TELEGRAM_BOT_API_TOKEN` | Telegram bot API token for notifications | Yes |
| `TELEGRAM_USER_ID` | Your Telegram user ID for receiving notifications | Yes |
| `GITHUB_USER_EMAIL` | GitHub user email for git operations | No |
| `GITHUB_USER_NAME` | GitHub username for git operations | No |
| `GITHUB_SSH_KEY` | Path to SSH key for GitHub access | No |
| `GITHUB_PERSONAL_ACCESS_TOKEN` | GitHub PAT for API access | No |
| `MINIMAX_API_KEY` | MiniMax API key for AI features | No |
| `PROTON_EMAIL_ENV_PATH` | Path to .env file (default: `/run/secrets/proton_email.env`) | No |
| `PROTON_EMAIL_DATA_DIR` | Directory for SQLite database (default: `/app/data`) | No |
| `MUCLAW_STATE_DIR` | Directory for application state (default: `/app/state`) | No |

### Volume Mounts

The Docker Compose configuration mounts the following volumes:

| Host Path | Container Path | Purpose |
|-----------|----------------|---------|
| `./data` | `/app/data` | SQLite database and persistent data |
| `muclaw_state` (named volume) | `/app/state` | Application worktree and state |
| `~/.env` | `/run/secrets/proton_email.env` | Secrets file (read-only) |

To persist data on the host:

```bash
# Create data directory
mkdir -p data

# Set appropriate permissions
chmod 755 data

# Start container
docker compose up -d
```

### Running Commands Manually

To run commands inside the container:

```bash
# Run a sync manually
docker compose exec proton-email proton-email sync

# List recent records
docker compose exec proton-email proton-email list

# Search emails
docker compose exec proton-email proton-email search "query"

# Open an interactive shell
docker compose exec proton-email /bin/bash
```

### Viewing Logs

```bash
# View container logs
docker compose logs -f

# View sync cron logs inside container
docker compose exec proton-email tail -f /var/log/proton_email_sync.log
```

### Health Check

The container starts cron in the foreground. To verify it's running:

```bash
docker compose ps
docker compose logs proton-email
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
| `PROTON_EMAIL_DATA_DIR` | `./data` | Directory for SQLite database |
| `PROTON_EMAIL_ENV_PATH` | `./.env` | Path to .env file |
| `MUCLAW_STATE_DIR` | `/app/state` | Directory for application state |
| `TELEGRAM_BOT_API_TOKEN` | - | Telegram bot API token for notifications |
| `TELEGRAM_USER_ID` | - | Telegram user ID for receiving notifications |
| `GITHUB_USER_EMAIL` | - | GitHub user email for git operations |
| `GITHUB_USER_NAME` | - | GitHub username for git operations |
| `GITHUB_SSH_KEY` | - | Path to SSH key for GitHub access |
| `MINIMAX_API_KEY` | - | MiniMax API key for AI features |

## Usage

```bash
uv run proton-email sync      # Sync data
uv run proton-email list [n]  # Show last N records (default: 5)
uv run proton-email search <query>  # Full-text search
uv run proton-email show <id>  # Show full details
```
