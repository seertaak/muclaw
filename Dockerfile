FROM python:3.12-slim

WORKDIR /app

# Install cron, git, curl, and Node.js (for Claude Code CLI)
RUN apt-get update && apt-get install -y --no-install-recommends \
    cron \
    git \
    curl \
    gnupg \
    && rm -rf /var/lib/apt/lists/*

# Install Node.js 20.x for Claude Code CLI
RUN curl -fsSL https://deb.nodesource.com/setup_20.x | bash - \
    && apt-get install -y nodejs \
    && rm -rf /var/lib/apt/lists/*

# Install Claude Code CLI
RUN npm install -g @anthropic-ai/claude-code

# Install gh CLI for GitHub integration
RUN curl -fsSL https://cli.github.com/packages/githubcli-archive-keyring.gpg | dd of=/usr/share/keyrings/githubcli-archive-keyring.gpg \
    && chmod go+r /usr/share/keyrings/githubcli-archive-keyring.gpg \
    && echo "deb [arch=$(dpkg --print-architecture) signed-by=/usr/share/keyrings/githubcli-archive-keyring.gpg] https://cli.github.com/packages stable main" | tee /etc/apt/sources.list.d/github-cli.list > /dev/null \
    && apt-get update \
    && apt-get install -y gh \
    && rm -rf /var/lib/apt/lists/*

# Copy requirements and install dependencies
COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

# Copy application source
COPY src/ ./src/
COPY pyproject.toml .

# Copy Claude skills to app directory
COPY .claude/ /app/.claude/

# Copy entrypoint script
COPY entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh

# Install the package
RUN pip install -e .

# Create state directory
RUN mkdir -p /app/state

# Create data directory
RUN mkdir -p /app/data

# Set up cron to run sync every minute
# The .env file is mounted at /run/secrets/proton_email.env
RUN echo "* * * * * root /usr/local/bin/python -m proton_email sync >> /var/log/proton_email_sync.log 2>&1" > /etc/cron.d/proton_email_sync
RUN chmod 0644 /etc/cron.d/proton_email_sync
RUN crontab /etc/cron.d/proton_email_sync

# Create log file
RUN touch /var/log/proton_email_sync.log

# Use entrypoint script for auto-configuration
ENTRYPOINT ["/entrypoint.sh"]
