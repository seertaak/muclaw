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

### Building the Image

Build the Docker image using Docker Compose:

```bash
docker compose build
```

Or build directly with docker:

```bash
docker build -t muclaw .
```

### Running the Container

#### Using Docker Compose (Recommended)

1. Ensure your secrets are configured in `docker-compose.yml` or as Docker secrets (see below)

2. Build and run with Docker Compose:

```bash
docker compose up -d
```

3. View logs:

```bash
docker compose logs -f
```

#### Using Docker Directly

```bash
docker run -d \
  --name muclaw \
  -v $(pwd)/data:/app/data \
  -v muclaw_state:/app/state \
  -e PROTON_EMAIL_DATA_DIR=/app/data \
  -e PROTON_EMAIL_ENV_PATH=/run/secrets/proton_email.env \
  --restart unless-stopped \
  muclaw
```

### Environment Variables

| Variable | Description |
|----------|-------------|
| `TELEGRAM_BOT_API_TOKEN` | Telegram bot API token for notifications |
| `TELEGRAM_USER_ID` | Telegram user ID for authorization |
| `GITHUB_USER_EMAIL` | GitHub user email for git operations |
| `GITHUB_USER_NAME` | GitHub username for git operations |
| `GITHUB_SSH_KEY` | Path to SSH private key for GitHub |
| `GITHUB_PERSONAL_ACCESS_TOKEN` | GitHub personal access token |
| `MINIMAX_API_KEY` | MiniMax API key for AI features |
| `PROTON_EMAIL_DATA_DIR` | Directory for SQLite database (default: `/app/data`) |
| `PROTON_EMAIL_ENV_PATH` | Path to .env file (default: `/run/secrets/proton_email.env`) |

### Passing Secrets

#### Method 1: File-based Secrets (Recommended for Development)

Mount your `.env` file as a Docker secret:

```yaml
# docker-compose.yml
services:
  proton-email:
    volumes:
      - ~/.env:/run/secrets/proton_email.env:ro
```

```bash
# Create your .env file with sensitive values
cat > ~/.env << EOF
TELEGRAM_BOT_API_TOKEN=your_telegram_token
TELEGRAM_USER_ID=your_user_id
GITHUB_USER_EMAIL=your@email.com
GITHUB_USER_NAME=your_username
GITHUB_SSH_KEY=/app/.ssh/id_ed25519
MINIMAX_API_KEY=your_minimax_key
EOF

# Run with docker compose
docker compose up -d
```

#### Method 2: Docker Secrets (Recommended for Production)

Create Docker secrets for sensitive data:

```bash
# Create secrets
echo "your_telegram_token" | docker secret create telegram_bot_api_token -
echo "your_user_id" | docker secret create telegram_user_id -
echo "your_minimax_key" | docker secret create minimax_api_key -
```

Use in your stack file:

```yaml
# stack.yml
services:
  proton-email:
    image: muclaw
    secrets:
      - telegram_bot_api_token
      - telegram_user_id
      - minimax_api_key
    environment:
      - TELEGRAM_BOT_API_TOKEN_FILE=/run/secrets/telegram_bot_api_token
      - TELEGRAM_USER_ID_FILE=/run/secrets/telegram_user_id
      - MINIMAX_API_KEY_FILE=/run/secrets/minimax_api_key

secrets:
  telegram_bot_api_token:
    external: true
  telegram_user_id:
    external: true
  minimax_api_key:
    external: true
```

Deploy the stack:

```bash
docker stack deploy -c stack.yml muclaw
```

### Volume Mounts for Data Persistence

The container uses the following volumes:

| Volume | Host Path | Description |
|--------|-----------|-------------|
| `/app/data` | `./data` or named volume | SQLite database and email cache |
| `/app/state` | named volume `muclaw_state` | Application state directory |
| `/run/secrets/proton_email.env` | `~/.env` or Docker secret | Secrets and environment variables |

#### Using Named Volumes (Default)

Docker Compose uses named volumes by default:

```yaml
volumes:
  muclaw_state:
```

#### Using Host Paths

For host-based persistence:

```yaml
services:
  proton-email:
    volumes:
      - /opt/muclaw/data:/app/data
      - /opt/muclaw/state:/app/state
```

### Cron Automation

When running without arguments, the container automatically:
- Starts cron in the foreground
- Executes `proton-email sync` every minute
- Logs sync output to `/var/log/proton_email_sync.log`

### Running Commands Interactively

To run a specific command instead of the scheduled sync:

```bash
# Using docker compose
docker compose run --rm proton-email proton-email sync

# Using docker directly
docker exec muclaw proton-email list 10
```

### Health Check

The container logs activity to `/var/log/proton_email_sync.log`. Monitor with:

```bash
docker compose logs -f proton-email
```

## Database

The SQLite database (`data/emails.db`) stores assistant data with:
- **emails**: Core data records
- **attachments**: Binary blobs for data attachments
- **emails_fts**: Full-text search index (FTS5)
- **sync_state**: Tracks incremental sync state

SQLite is configured with WAL mode for safe concurrent access.

## Usage

```bash
uv run proton-email sync      # Sync data
uv run proton-email list [n]  # Show last N records (default: 5)
uv run proton-email search <query>  # Full-text search
uv run proton-email show <id>  # Show full details
```
