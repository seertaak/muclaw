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

This project is designed to run as a Docker container with automatic syncing via cron.

### Building the Image

Build the Docker image:

```bash
docker build -t muclaw .
```

Or build and run with Docker Compose:

```bash
docker compose up -d --build
```

### Configuration

Create a `.env` file containing your secrets. The container expects secrets at `/run/secrets/proton_email.env` (mounted from your local `~/.env`).

#### Environment Variables

| Variable | Description |
|----------|-------------|
| `TELEGRAM_BOT_API_TOKEN` | Telegram bot API token for notifications |
| `TELEGRAM_USER_ID` | Your Telegram user ID for bot access |
| `GITHUB_USER_EMAIL` | GitHub user email for git operations |
| `GITHUB_USER_NAME` | GitHub username for git operations |
| `GITHUB_SSH_KEY` | Path to SSH private key for GitHub access |
| `GITHUB_PERSONAL_ACCESS_TOKEN` | GitHub personal access token |
| `MINIMAX_API_KEY` | MiniMax API key for AI features |

#### Example `.env` file

```bash
# Telegram Configuration
TELEGRAM_BOT_API_TOKEN=your_bot_token_here
TELEGRAM_USER_ID=123456789

# GitHub Configuration
GITHUB_USER_EMAIL=your@email.com
GITHUB_USER_NAME=yourusername
GITHUB_SSH_KEY=/root/.ssh/id_ed25519
GITHUB_PERSONAL_ACCESS_TOKEN=ghp_xxxxxxxxxxxx

# AI Services
MINIMAX_API_KEY=your_minimax_api_key
```

### Running the Container

1. Place your `.env` file in your home directory:

```bash
cp .env ~/.env
```

2. Start the container with Docker Compose:

```bash
docker compose up -d
```

The container will:
- Load secrets from `~/.env` (mounted as a Docker secret)
- Start cron to run sync automatically every minute
- Mount `./data` for database persistence
- Mount a named volume (`muclaw_state`) for application state

### Docker Secrets

The container uses Docker's secret mounting mechanism. The entrypoint script loads environment variables from `/run/secrets/proton_email.env`.

With Docker Compose, mount your `.env` file:

```yaml
volumes:
  - ~/.env:/run/secrets/proton_email.env:ro
```

### Volume Mounts

The following volumes are configured for data persistence:

| Host Path | Container Path | Purpose |
|-----------|---------------|---------|
| `./data` | `/app/data` | SQLite database and attachments |
| `muclaw_state` (named volume) | `/app/state` | Application state and worktrees |
| `~/.env` | `/run/secrets/proton_email.env:ro` | Secrets (read-only) |

### Viewing Logs

Check sync logs:

```bash
docker compose logs -f
```

View cron execution logs inside the container:

```bash
docker exec muclaw-proton-email-1 tail -f /var/log/proton_email_sync.log
```

### Running Commands Manually

To run a single command inside the container:

```bash
docker compose exec proton-email proton-email sync
docker compose exec proton-email proton-email list
docker compose exec proton-email proton-email search "query"
```

### Stopping the Container

```bash
docker compose down
```

To also remove volumes:

```bash
docker compose down -v
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
