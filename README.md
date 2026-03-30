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

### Prerequisites

- Docker Engine 20.10+
- Docker Compose v2.0+ (or use `docker compose` as shown below)

### Building the Image

Build the Docker image using Docker Compose:

```bash
docker compose build
```

Or build directly with `docker build`:

```bash
docker build -t proton-email:latest .
```

### Running the Container

1. Ensure your environment variables are configured in `~/.env`:

```bash
# Required for Telegram notifications
TELEGRAM_BOT_API_TOKEN=your_telegram_bot_token
TELEGRAM_USER_ID=your_telegram_user_id

# Required for GitHub integration
GITHUB_USER_EMAIL=your.email@example.com
GITHUB_USER_NAME=Your Name
GITHUB_SSH_KEY=/run/secrets/.ssh/id_ed25519
GITHUB_PERSONAL_ACCESS_TOKEN=your_github_token

# Required for AI features
MINIMAX_API_KEY=your_minimax_api_key
```

2. Start the container:

```bash
docker compose up -d
```

The container will:
- Load secrets from `~/.env` mounted at `/run/secrets/proton_email.env`
- Configure git with your credentials
- Start cron to sync data every minute automatically

### Passing Secrets

The application uses Docker secrets to handle sensitive configuration:

| Secret File | Purpose |
|-------------|---------|
| `~/.env:/run/secrets/proton_email.env:ro` | Main environment variables |

The entrypoint script (`entrypoint.sh`) loads and exports these variables at container startup.

### Volume Mounts

The following volumes are configured for data persistence:

| Host Path | Container Path | Purpose |
|-----------|---------------|---------|
| `./data` | `/app/data` | SQLite database and email cache |
| `muclaw_state` (named volume) | `/app/state` | Application state and logs |

To persist data on your host, modify `docker-compose.yml` to use a host path:

```yaml
volumes:
  - /path/to/your/data:/app/data
  - /path/to/state:/app/state
  - ~/.env:/run/secrets/proton_email.env:ro
```

### Viewing Logs

Check sync logs:

```bash
docker compose logs -f
```

Or access the sync log directly:

```bash
docker compose exec proton-email cat /var/log/proton_email_sync.log
```

### Running Commands Manually

Execute CLI commands inside the container:

```bash
# Sync data
docker compose exec proton-email proton-email sync

# List recent records
docker compose exec proton-email proton-email list 10

# Search emails
docker compose exec proton-email proton-email search "subject:urgent"

# Show specific record
docker compose exec proton-email proton-email show <id>
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
