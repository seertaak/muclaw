#include "vector_db.hpp"
#include "logger.hpp"

#include <stdexcept>

namespace nclaw {

VectorDatabase::VectorDatabase(int dimension)
    : dimension_{dimension}, flat_index_{dimension}, id_map_{&flat_index_} {
    log::info("Initialized VectorDatabase with dimension {}", dimension_);
}

auto VectorDatabase::add(std::vector<float> const& embedding, int64_t id) -> void {
    if (embedding.size() != static_cast<size_t>(dimension_)) {
        throw std::invalid_argument("Embedding dimension mismatch");
    }
    
    // faiss::IndexIDMap takes ids as faiss::idx_t (which is usually int64_t)
    id_map_.add_with_ids(1, embedding.data(), &id);
}

auto VectorDatabase::search(std::vector<float> const& query, int k) const -> std::vector<SearchResult> {
    if (query.size() != static_cast<size_t>(dimension_)) {
        throw std::invalid_argument("Query dimension mismatch");
    }

    if (id_map_.ntotal == 0) {
        return {};
    }

    // Limit k to the number of available items
    int actual_k = std::min(k, static_cast<int>(id_map_.ntotal));

    std::vector<float> distances(actual_k);
    std::vector<faiss::idx_t> indices(actual_k);

    id_map_.search(1, query.data(), actual_k, distances.data(), indices.data());

    std::vector<SearchResult> results;
    results.reserve(actual_k);
    for (int i = 0; i < actual_k; ++i) {
        if (indices[i] != -1) { // -1 means not found (happens if not enough elements)
            results.push_back({indices[i], distances[i]});
        }
    }

    return results;
}

auto VectorDatabase::count() const -> size_t {
    return id_map_.ntotal;
}

} // namespace nclaw
