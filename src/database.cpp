#include "database.hpp"
#include "logger.hpp"

#include <rocksdb/options.h>
#include <rocksdb/status.h>

#include <stdexcept>

namespace muclaw {

Database::~Database() {
    close();
}

auto Database::open(std::string_view path) -> void {
    rocksdb::Options opt{};
    opt.create_if_missing = true;

    rocksdb::DB* raw{};
    rocksdb::Status s = rocksdb::DB::Open(opt, std::string{path}, &raw);
    if (!s.ok()) {
        log::error("RocksDB open failed: {}", s.ToString());
        throw std::runtime_error(fmt::format("Failed to open db: {}", s.ToString()));
    }

    db.reset(raw);
    log::info("RocksDB opened successfully at {}", path);
}

auto Database::close() -> void {
    if (db) {
        db->Close();
        db.reset();
        log::info("RocksDB closed.");
    }
}

auto Database::put(std::string_view k, std::string_view v) -> void {
    if (!db)
        return;
    rocksdb::Status s =
        db->Put(rocksdb::WriteOptions{}, rocksdb::Slice{k.data(), k.size()}, rocksdb::Slice{v.data(), v.size()});
    if (!s.ok()) {
        log::warn("RocksDB put failed: {}", s.ToString());
    }
}

auto Database::value(std::string_view k) const -> std::optional<std::string> {
    if (!db)
        return std::nullopt;
    std::string v{};
    rocksdb::Status s = db->Get(rocksdb::ReadOptions{}, rocksdb::Slice{k.data(), k.size()}, &v);
    if (s.IsNotFound()) {
        return std::nullopt;
    }
    if (!s.ok()) {
        log::warn("RocksDB get failed: {}", s.ToString());
        return std::nullopt;
    }
    return v;
}

} // namespace muclaw
