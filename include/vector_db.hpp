#pragma once

#include <cstddef>
#include <faiss/IndexFlat.h>
#include <faiss/IndexIDMap.h>

#include <cstdint>
#include <vector>

#include <boost/asio/awaitable.hpp>
#include <boost/asio/thread_pool.hpp>

namespace muclaw {

struct SearchResult {
    int64_t id;
    float distance;
};

class VectorDB {
public:
    explicit VectorDB(boost::asio::thread_pool& pool, int dimension = 1536); // Default assuming OpenAI embeddings

    auto add(std::vector<float> const& embedding, int64_t id) -> void;
    auto search(std::vector<float> const& query, int k = 5) const -> std::vector<SearchResult>;
    auto count() const -> size_t;

    auto add_async(std::vector<float> embedding, int64_t id) -> boost::asio::awaitable<void>;
    auto search_async(std::vector<float> query, int k = 5) const -> boost::asio::awaitable<std::vector<SearchResult>>;

private:
    boost::asio::thread_pool& pool_;
    int dimension_;
    faiss::IndexFlatL2 flat_index_;
    faiss::IndexIDMap id_map_;
};

} // namespace muclaw
