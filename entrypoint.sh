#!/bin/bash
set -e

# Source secrets from Docker secret and export them
if [ -f /run/secrets/proton_email.env ]; then
    echo "[Entrypoint] Loading secrets from /run/secrets/proton_email.env"
    set -a
    source /run/secrets/proton_email.env
    set +a
    export TELEGRAM_BOT_API_TOKEN
    export TELEGRAM_USER_ID
    export GITHUB_USER_EMAIL
    export GITHUB_USER_NAME
    export GITHUB_SSH_KEY
fi

# Configure git if credentials provided
if [ -n "$GITHUB_USER_EMAIL" ]; then
    echo "[Entrypoint] Configuring git user.email=$GITHUB_USER_EMAIL"
    git config --global user.email "$GITHUB_USER_EMAIL"
fi

if [ -n "$GITHUB_USER_NAME" ]; then
    echo "[Entrypoint] Configuring git user.name=$GITHUB_USER_NAME"
    git config --global user.name "$GITHUB_USER_NAME"
fi

# Configure SSH for GitHub if key path provided
if [ -n "$GITHUB_SSH_KEY" ]; then
    echo "[Entrypoint] Configuring SSH for GitHub with key: $GITHUB_SSH_KEY"
    git config --global core.sshCommand "ssh -i '$GITHUB_SSH_KEY' -o IdentitiesOnly=yes"
    git config --global url."git@github.com:".insteadOf "https://github.com/"
    echo "[Entrypoint] Git configured to use SSH key: $GITHUB_SSH_KEY"
fi

# Verify gh can talk to GitHub
if gh auth status &>/dev/null; then
    echo "[Entrypoint] gh is authenticated"
else
    echo "[Entrypoint] gh may not be authenticated, but SSH is configured for git operations"
fi

# If arguments passed, execute them (e.g., "bash", "muclaw hello", etc.)
if [ $# -gt 0 ]; then
    echo "[Entrypoint] Executing: $*"
    exec "$@"
fi

# Otherwise, start cron in foreground
echo "[Entrypoint] Starting cron..."
exec cron -f
