#pragma once

#include <filesystem>
#include <memory>
#include <string_view>
#include <vector>

#include <common/common.h>

namespace muclaw {

namespace fs = std::filesystem;

using embeddings_t = std::vector<float>;

class LocalEmbedder {
    explicit LocalEmbedder(fs::path const& model, int n_threads);

public:
    LocalEmbedder(LocalEmbedder const&) = delete;
    auto operator=(LocalEmbedder const&) -> LocalEmbedder& = delete;
    LocalEmbedder(LocalEmbedder&&) = delete;
    auto operator=(LocalEmbedder&&) -> LocalEmbedder& = delete;

    auto embed(std::vector<std::string> const& prompts) -> std::vector<float>;
    auto embed(std::string_view const& text) -> std::vector<float> {
        return embed(std::vector{std::string{text}});
    }

    static void init(fs::path const& model, int n_threads = 1);
    static void init(int n_threads = 1);
    static auto instance() -> LocalEmbedder&;

    const fs::path model_path;

private:
    common_params params_;
    common_init_result llama_;

    static std::unique_ptr<LocalEmbedder> instance_;
};

} // namespace muclaw
