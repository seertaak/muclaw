#include "database.hpp"
#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <string>

TEST_CASE("Database extensive operations", "[database]") {
    std::filesystem::remove("test_db.sqlite");

    muclaw::Database db;

    // SQLite creates the file if it doesn't exist
    REQUIRE_NOTHROW(db.open("test_db.sqlite"));

    SECTION("Users and counterparties") {
        REQUIRE_NOTHROW(db.execute("INSERT INTO counterparties (name, type) VALUES ('Dr. John', 'doctor')"));
        REQUIRE_NOTHROW(db.execute("INSERT INTO counterparties (name, type) VALUES ('My Assistant', 'assistant')"));

        auto assistant_id = db.query_first_string("SELECT id FROM counterparties WHERE name = 'My Assistant'");
        REQUIRE(assistant_id.has_value());
        REQUIRE(*assistant_id == "2"); // sqlite sequence
    }

    SECTION("Threads and FTS5") {
        REQUIRE_NOTHROW(
            db.execute("INSERT INTO threads (subject, channel) VALUES ('Medical Diagnosis Discussion', 'telegram')"));

        auto thread_id = db.query_first_string("SELECT id FROM threads WHERE channel = 'telegram'");
        REQUIRE(thread_id.has_value());
        REQUIRE(*thread_id == "1");

        // Verify FTS5 Trigger on threads
        auto fts_search = db.query_first_string("SELECT rowid FROM threads_fts WHERE threads_fts MATCH 'diagnosis'");
        REQUIRE(fts_search.has_value());
        REQUIRE(*fts_search == "1");

        auto no_match = db.query_first_string("SELECT rowid FROM threads_fts WHERE threads_fts MATCH 'apples'");
        REQUIRE(!no_match.has_value());
    }

    SECTION("Messages and chunks FTS5") {
        REQUIRE_NOTHROW(db.execute("INSERT INTO messages (thread_id, sender_id, recipient_id, channel, raw_content) "
                                   "VALUES (1, 1, 2, 'telegram', 'Hello this is doctor John.')"));

        REQUIRE_NOTHROW(db.execute("INSERT INTO message_chunks (message_id, chunk_index, content) VALUES (1, 0, 'This "
                                   "is an important chunk about cardiology.')"));
        REQUIRE_NOTHROW(db.execute("INSERT INTO message_chunks (message_id, chunk_index, content) VALUES (1, 1, "
                                   "'Another chunk about a prescription for medication.')"));

        // Verify FTS5 Triggers on message_chunks
        auto chunk_match =
            db.query_first_string("SELECT rowid FROM message_chunks_fts WHERE message_chunks_fts MATCH 'cardiology'");
        REQUIRE(chunk_match.has_value());
        REQUIRE(*chunk_match == "1"); // chunk_id 1

        auto chunk_match_2 =
            db.query_first_string("SELECT rowid FROM message_chunks_fts WHERE message_chunks_fts MATCH 'prescription'");
        REQUIRE(chunk_match_2.has_value());
        REQUIRE(*chunk_match_2 == "2");

        // Verify update trigger handles replacement
        REQUIRE_NOTHROW(db.execute("UPDATE message_chunks SET content = 'Now it is about neurology' WHERE id = 1"));

        auto old_match =
            db.query_first_string("SELECT rowid FROM message_chunks_fts WHERE message_chunks_fts MATCH 'cardiology'");
        REQUIRE(!old_match.has_value()); // cardiology no longer exists

        auto new_match =
            db.query_first_string("SELECT rowid FROM message_chunks_fts WHERE message_chunks_fts MATCH 'neurology'");
        REQUIRE(new_match.has_value());
        REQUIRE(*new_match == "1");
    }

    REQUIRE_NOTHROW(db.close());
    std::filesystem::remove("test_db.sqlite");
}
