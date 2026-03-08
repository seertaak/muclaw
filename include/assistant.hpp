#pragma once

#include "database.hpp"
#include "llm.hpp"
#include "telegram.hpp"
#include "vector_db.hpp"

#include <boost/asio/awaitable.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/thread_pool.hpp>

#include <string>

namespace muclaw {

class Assistant {
public:
    Assistant(boost::asio::io_context& io, std::string telegram_token, std::string llm_host, std::string llm_api_key);

    auto start() -> boost::asio::awaitable<void>;

private:
    auto handle_message(int64_t chat_id, std::string text) -> boost::asio::awaitable<void>;

    Database db_;
    boost::asio::thread_pool worker_pool_;
    VectorDB vdb_;
    LlmClient llm_;
    TelegramClient tg_;
};

} // namespace muclaw
