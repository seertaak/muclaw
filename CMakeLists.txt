cmake_minimum_required(VERSION 3.24)
project(muclaw VERSION 0.1.0 LANGUAGES CXX)

# Enforce C++23
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS False)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# For Conan 2.x CMakeToolchain
find_package(Boost REQUIRED COMPONENTS system thread coroutine context)
find_package(SQLite3 REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(faiss REQUIRED)
find_package(fmt REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Catch2 REQUIRED)
find_package(CLI11 REQUIRED)
find_package(llama-cpp REQUIRED)

add_library(muclaw_core
    src/database.cpp
    src/telegram.cpp
    src/llm.cpp
    src/vector_db.cpp
    src/asio_curl.cpp
    src/assistant.cpp
    src/local_embedder.cpp
)

target_include_directories(muclaw_core PUBLIC include)

target_link_libraries(muclaw_core PUBLIC
    Boost::context
    Boost::thread
    Boost::system
    SQLite::SQLite3
    nlohmann_json::nlohmann_json
    faiss
    fmt::fmt
    openssl::openssl
    CLI11::CLI11
    llama-cpp::llama-cpp
)

if(MSVC)
    target_compile_options(muclaw_core PUBLIC /W4 /WX)
else()
    target_compile_options(muclaw_core PUBLIC -Wall -Wextra -Wpedantic -Werror)
endif()

# Main Executable
add_executable(muclaw src/main.cpp)
target_link_libraries(muclaw PRIVATE muclaw_core)

# Tests
enable_testing()
add_executable(muclaw_tests
    tests/test_database.cpp
    tests/test_vector_db.cpp
    tests/test_asio_curl.cpp
    tests/test_local_embedder.cpp
)
target_compile_definitions(muclaw_core PUBLIC MUCLAW_ASSETS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets")
target_compile_definitions(muclaw_tests PRIVATE MUCLAW_ASSETS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets")
target_link_libraries(muclaw_tests PRIVATE muclaw_core Catch2::Catch2WithMain)

include(Catch)
catch_discover_tests(muclaw_tests)
