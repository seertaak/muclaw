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

Build the image using Docker Compose (recommended):

```bash
docker compose build
```

Or build directly with Docker:

```bash
docker build -t muclaw .
```

### Running the Container

#### Option 1: Using Docker Compose (Recommended)

1. Ensure your `.env` file is in your home directory (`~/.env`):

```bash
# Example .env file
TELEGRAM_BOT_API_TOKEN=your_telegram_bot_token
TELEGRAM_USER_ID=your_telegram_user_id
GITHUB_USER_EMAIL=your@email.com
GITHUB_USER_NAME=YourName
GITHUB_SSH_KEY=/run/secrets/github_ssh_key
GITHUB_PERSONAL_ACCESS_TOKEN=your_github_token
MINIMAX_API_KEY=your_minimax_api_key
```

2. Build and run with Docker Compose:

```bash
docker compose up -d
```

#### Option 2: Using Docker Secrets

For production deployments, use Docker secrets for sensitive data:

1. Create a secrets file at `/run/secrets/proton_email.env` or `/run/secrets/secrets.env`:

```bash
# Create the secrets directory
sudo mkdir -p /run/secrets

# Create the secrets file
sudo tee /run/secrets/proton_email.env << 'EOF'
TELEGRAM_BOT_API_TOKEN=your_telegram_bot_token
TELEGRAM_USER_ID=your_telegram_user_id
GITHUB_USER_EMAIL=your@email.com
GITHUB_USER_NAME=YourName
GITHUB_SSH_KEY=/run/secrets/github_ssh_key
GITHUB_PERSONAL_ACCESS_TOKEN=your_github_token
MINIMAX_API_KEY=your_minimax_api_key
EOF

sudo chmod 600 /run/secrets/proton_email.env
```

2. Run the container with the secrets:

```bash
docker run -d \
  --name muclaw \
  -v /run/secrets/proton_email.env:/run/secrets/proton_email.env:ro \
  -v ./data:/app/data \
  -v muclaw_state:/app/state \
  -e PROTON_EMAIL_DATA_DIR=/app/data \
  -e PROTON_EMAIL_ENV_PATH=/run/secrets/proton_email.env \
  muclaw
```

### Environment Variables

| Variable | Description |
|----------|-------------|
| `PROTON_EMAIL_DATA_DIR` | Directory for SQLite database (default: `/app/data`) |
| `PROTON_EMAIL_ENV_PATH` | Path to environment file (default: `/run/secrets/proton_email.env`) |
| `MUCLAW_STATE_DIR` | Directory for agent state (default: `/app/state`) |

### Volume Mounts

The container uses the following volumes:

| Volume | Host Path | Description |
|--------|-----------|-------------|
| Data | `./data:/app/data` | SQLite database and email data |
| State | `muclaw_state:/app/state` | Agent state (worktrees, etc.) |
| Secrets | `~/.env:/run/secrets/proton_email.env:ro` | Environment variables (read-only) |

### Automatic Sync

When running without arguments, the container automatically runs a sync every minute via cron. To run manual commands:

```bash
# Run a sync command
docker compose exec proton-email python -m proton_email sync

# List recent records
docker compose exec proton-email python -m proton_email list 10

# Search emails
docker compose exec proton-email python -m proton_email search "keyword"
```

### Logs

View container logs:

```bash
docker compose logs -f
```

View sync logs inside the container:

```bash
docker compose exec proton-email tail -f /var/log/proton_email_sync.log
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
