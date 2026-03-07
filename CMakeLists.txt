cmake_minimum_required(VERSION 3.24)
project(muclaw VERSION 0.1.0 LANGUAGES CXX)

# Enforce C++23
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS False)

# For Conan 2.x CMakeToolchain
find_package(Boost REQUIRED COMPONENTS system thread coroutine context)
find_package(RocksDB REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(faiss REQUIRED)
find_package(fmt REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Catch2 REQUIRED)
find_package(CLI11 REQUIRED)

add_library(nanoclaw_core
    src/database.cpp
    src/telegram.cpp
    src/llm.cpp
    src/vector_db.cpp
    src/asio_curl.cpp
    src/assistant.cpp
)

target_include_directories(nanoclaw_core PUBLIC include)

target_link_libraries(nanoclaw_core PUBLIC
    Boost::context
    Boost::thread
    Boost::system
    RocksDB::rocksdb
    nlohmann_json::nlohmann_json
    faiss
    fmt::fmt
    openssl::openssl
    CLI11::CLI11
)

if(MSVC)
    target_compile_options(nanoclaw_core PUBLIC /W4 /WX)
else()
    target_compile_options(nanoclaw_core PUBLIC -Wall -Wextra -Wpedantic -Werror)
endif()

# Main Executable
add_executable(nanoclaw src/main.cpp)
target_link_libraries(nanoclaw PRIVATE nanoclaw_core)

# Tests
enable_testing()
add_executable(nanoclaw_tests
    tests/test_database.cpp
    tests/test_vector_db.cpp
    tests/test_asio_curl.cpp
)
target_link_libraries(nanoclaw_tests PRIVATE nanoclaw_core Catch2::Catch2WithMain)

include(Catch)
catch_discover_tests(nanoclaw_tests)
