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
# Build the image
docker build -t muclaw .

# Or use Docker Compose to build and run
docker compose up -d --build
```

### Running the Container

The application runs in two modes:

1. **Background mode (default)**: Starts cron which runs email sync every minute
2. **Interactive mode**: Run CLI commands directly

```bash
# Background mode with Docker Compose (recommended)
docker compose up -d

# Interactive mode - run a specific command
docker compose run --rm proton-email sync
docker compose run --rm proton-email list 10
docker compose run --rm proton-email search "important"
```

### Environment Variables

The following environment variables are supported:

| Variable | Description |
|----------|-------------|
| `TELEGRAM_BOT_API_TOKEN` | Telegram bot token for notifications |
| `TELEGRAM_USER_ID` | Your Telegram user ID for authentication |
| `GITHUB_USER_EMAIL` | GitHub user email for git operations |
| `GITHUB_USER_NAME` | GitHub username for git operations |
| `GITHUB_SSH_KEY` | Path to SSH private key for GitHub access |
| `GITHUB_PERSONAL_ACCESS_TOKEN` | GitHub personal access token |
| `MINIMAX_API_KEY` | Minimax API key for AI features |

### Managing Secrets

#### Option 1: Using a .env file (Recommended for local development)

Create a `.env` file in your home directory:

```bash
# ~/.env
TELEGRAM_BOT_API_TOKEN=your_telegram_bot_token
TELEGRAM_USER_ID=your_telegram_user_id
GITHUB_USER_EMAIL=your@email.com
GITHUB_USER_NAME=your_username
GITHUB_SSH_KEY=/root/.ssh/id_ed25519
```

The `docker-compose.yml` mounts `~/.env` to `/run/secrets/proton_email.env` automatically.

#### Option 2: Using Docker secrets (Production)

For swarm mode deployments, use Docker secrets:

```yaml
# docker-compose.yml
services:
  proton-email:
    secrets:
      - source: proton_secrets
        target: proton_email.env
    # ...

secrets:
  proton_secrets:
    file: ./secrets/proton_email.env
```

Create the secrets file:

```bash
mkdir -p secrets
cat > secrets/proton_email.env << EOF
TELEGRAM_BOT_API_TOKEN=your_telegram_bot_token
TELEGRAM_USER_ID=your_telegram_user_id
GITHUB_USER_EMAIL=your@email.com
GITHUB_USER_NAME=your_username
GITHUB_SSH_KEY=/run/secrets/id_ed25519
EOF
```

### Data Persistence

The following volumes are mounted for data persistence:

| Host Path | Container Path | Purpose |
|-----------|----------------|---------|
| `./data` | `/app/data` | SQLite database and email cache |
| `muclaw_state` (named volume) | `/app/state` | Application state and worktrees |
| `~/.env` | `/run/secrets/proton_email.env` | Secrets (read-only) |

**Important**: The `./data` directory is mounted from the host to persist the SQLite database. Ensure this directory exists and is writable:

```bash
mkdir -p data
chmod 755 data
```

### Viewing Logs

```bash
# View container logs
docker compose logs -f

# View sync logs inside container
docker compose exec proton-email cat /var/log/proton_email_sync.log
```

### Stopping the Container

```bash
docker compose down
```

To also remove volumes (WARNING: this will delete all data):

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
