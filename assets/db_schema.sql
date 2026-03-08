-- counterparties
CREATE TABLE IF NOT EXISTS counterparties (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL,
    type TEXT NOT NULL,
    external_identifier TEXT
);

-- threads
CREATE TABLE IF NOT EXISTS threads (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    subject TEXT,
    channel TEXT NOT NULL,
    external_thread_id TEXT,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME DEFAULT CURRENT_TIMESTAMP
);

-- messages
CREATE TABLE IF NOT EXISTS messages (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    thread_id INTEGER,
    sender_id INTEGER,
    recipient_id INTEGER,
    channel TEXT NOT NULL,
    external_msg_id TEXT,
    raw_content TEXT,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY(thread_id) REFERENCES threads(id),
    FOREIGN KEY(sender_id) REFERENCES counterparties(id),
    FOREIGN KEY(recipient_id) REFERENCES counterparties(id)
);

-- message_chunks
CREATE TABLE IF NOT EXISTS message_chunks (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    message_id INTEGER NOT NULL,
    chunk_index INTEGER NOT NULL,
    content TEXT NOT NULL,
    embedding_id INTEGER,
    FOREIGN KEY(message_id) REFERENCES messages(id)
);

-- message_chunks FTS virtual table
CREATE VIRTUAL TABLE IF NOT EXISTS message_chunks_fts USING fts5(
    content,
    content='', -- mapped to message_chunks.content via triggers later if desired
    tokenize='porter'
);

-- threads FTS virtual table for subject search
CREATE VIRTUAL TABLE IF NOT EXISTS threads_fts USING fts5(
    subject,
    content='',
    tokenize='porter'
);

CREATE TRIGGER IF NOT EXISTS message_chunks_ai 
AFTER INSERT ON message_chunks BEGIN
    INSERT INTO message_chunks_fts(rowid, content) VALUES (new.id, new.content);
END;

CREATE TRIGGER IF NOT EXISTS message_chunks_ad 
AFTER DELETE ON message_chunks BEGIN
    INSERT INTO message_chunks_fts(message_chunks_fts, rowid, content) VALUES('delete', old.id, old.content);
END;

CREATE TRIGGER IF NOT EXISTS message_chunks_au 
AFTER UPDATE ON message_chunks BEGIN
    INSERT INTO message_chunks_fts(message_chunks_fts, rowid, content) VALUES('delete', old.id, old.content);
    INSERT INTO message_chunks_fts(rowid, content) VALUES (new.id, new.content);
END;

CREATE TRIGGER IF NOT EXISTS threads_ai 
AFTER INSERT ON threads BEGIN
    INSERT INTO threads_fts(rowid, subject) VALUES (new.id, new.subject);
END;
