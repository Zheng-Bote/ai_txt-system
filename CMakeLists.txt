# cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
# cmake --build build -j"$(nproc)"

cmake_minimum_required(VERSION 3.28)
project(ai_txt_system
VERSION 1.0.0
DESCRIPTION "Consolidated AI TXT System"
HOMEPAGE_URL "https://github.com/Zheng-Bote/ai_txt-system"
LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

# 1. Asio (Standalone)
FetchContent_Declare(
    asio
    GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
    GIT_TAG        asio-1-30-2
)
FetchContent_MakeAvailable(asio)
set(ASIO_INCLUDE_DIR ${asio_SOURCE_DIR}/asio/include)

# 2. Crow (Microframework)
FetchContent_Declare(
    crow
    GIT_REPOSITORY https://github.com/CrowCpp/Crow.git
    GIT_TAG        master
)
FetchContent_MakeAvailable(crow)

find_package(CURL REQUIRED)
find_package(nlohmann_json REQUIRED)

include_directories(include)

set(COMMON_SOURCES src/llm_provider.cpp)

# CLI Version
add_executable(ai_cli src/ai_cli.cpp ${COMMON_SOURCES})
target_link_libraries(ai_cli PRIVATE CURL::libcurl nlohmann_json::nlohmann_json)

# Microservice Version
add_executable(ai_srv src/ai_srv.cpp ${COMMON_SOURCES})
target_link_libraries(ai_srv PRIVATE 
    CURL::libcurl 
    nlohmann_json::nlohmann_json 
    Crow::Crow
)
target_include_directories(ai_srv PRIVATE ${ASIO_INCLUDE_DIR})

file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/libs.txt" 
     CONTENT "$<TARGET_PROPERTY:ai_srv,LINK_LIBRARIES>")
