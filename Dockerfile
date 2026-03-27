FROM python:3.12-slim

WORKDIR /app

# Install cron and other utilities
RUN apt-get update && apt-get install -y --no-install-recommends \
    cron \
    && rm -rf /var/lib/apt/lists/*

# Copy requirements and install dependencies
COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

# Copy application source
COPY src/ ./src/
COPY pyproject.toml .

# Install the package
RUN pip install -e .

# Create data directory
RUN mkdir -p /app/data

# Set up cron to run sync every minute
# The .env file is mounted at /run/secrets/proton_email.env
RUN echo "* * * * * root /usr/local/bin/python -m proton_email sync >> /var/log/proton_email_sync.log 2>&1" > /etc/cron.d/proton_email_sync
RUN chmod 0644 /etc/cron.d/proton_email_sync
RUN crontab /etc/cron.d/proton_email_sync

# Create log file
RUN touch /var/log/proton_email_sync.log

# Start cron in foreground
CMD ["cron", "-f"]
