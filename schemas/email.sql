-- Emails table
CREATE TABLE IF NOT EXISTS emails (
    id INTEGER PRIMARY KEY,
    message_id TEXT UNIQUE,
    subject TEXT,
    from_addr TEXT,
    to_addr TEXT,
    date TIMESTAMP,
    body_text TEXT,
    body_html TEXT,
    raw_headers TEXT,
    received_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    is_read BOOLEAN DEFAULT 0,
    is_synced BOOLEAN DEFAULT 0
);

-- Attachments table
CREATE TABLE IF NOT EXISTS attachments (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    email_id INTEGER NOT NULL,
    filename TEXT,
    content_type TEXT,
    data BLOB NOT NULL,
    size INTEGER NOT NULL,
    FOREIGN KEY (email_id) REFERENCES emails(id) ON DELETE CASCADE
);

-- Sync state table
CREATE TABLE IF NOT EXISTS sync_state (
    id INTEGER PRIMARY KEY CHECK (id = 1),
    last_uid INTEGER DEFAULT 0,
    last_sync TIMESTAMP
);

-- FTS5 virtual table for full-text search
CREATE VIRTUAL TABLE IF NOT EXISTS emails_fts USING fts5(
    subject,
    body_text,
    from_addr,
    content='emails',
    content_rowid='id'
);

-- Triggers to keep FTS in sync
CREATE TRIGGER IF NOT EXISTS emails_ai AFTER INSERT ON emails BEGIN
    INSERT INTO emails_fts(rowid, subject, body_text, from_addr)
    VALUES (new.id, new.subject, new.body_text, new.from_addr);
END;

CREATE TRIGGER IF NOT EXISTS emails_ad AFTER DELETE ON emails BEGIN
    INSERT INTO emails_fts(emails_fts, rowid, subject, body_text, from_addr)
    VALUES ('delete', old.id, old.subject, old.body_text, old.from_addr);
END;

CREATE TRIGGER IF NOT EXISTS emails_au AFTER UPDATE ON emails BEGIN
    INSERT INTO emails_fts(emails_fts, rowid, subject, body_text, from_addr)
    VALUES ('delete', old.id, old.subject, old.body_text, old.from_addr);
    INSERT INTO emails_fts(rowid, subject, body_text, from_addr)
    VALUES (new.id, new.subject, new.body_text, new.from_addr);
END;