#pragma once

#include <faiss/IndexFlat.h>
#include <faiss/IndexIDMap.h>

#include <cstdint>
#include <vector>

namespace muclaw {

struct SearchResult {
    int64_t id;
    float distance;
};

class VectorDatabase {
public:
    explicit VectorDatabase(int dimension = 1536); // Default assuming OpenAI embeddings

    auto add(std::vector<float> const& embedding, int64_t id) -> void;
    auto search(std::vector<float> const& query, int k = 5) const -> std::vector<SearchResult>;
    auto count() const -> size_t;

private:
    int dimension_;
    faiss::IndexFlatL2 flat_index_;
    faiss::IndexIDMap id_map_;
};

} // namespace muclaw
