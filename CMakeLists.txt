# conan install . --output-folder=build --build=missing
# cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=build/conan_toolchain.cmake
# cmake --build build -j"$(nproc)"

cmake_minimum_required(VERSION 3.28)
project(ai_txt_system
VERSION 1.2.0
DESCRIPTION "Consolidated AI TXT System with Conan"
HOMEPAGE_URL "https://github.com/Zheng-Bote/ai_txt-system"
LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Ensure CMake finds Conan packages
set(CONAN_GENERATORS_DIR "${CMAKE_BINARY_DIR}/build/Release/generators")
if(NOT EXISTS "${CONAN_GENERATORS_DIR}")
    set(CONAN_GENERATORS_DIR "${CMAKE_BINARY_DIR}/build/Debug/generators")
endif()
list(APPEND CMAKE_PREFIX_PATH "${CONAN_GENERATORS_DIR}")

# Find Conan Packages
find_package(CURL REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(Crow REQUIRED)
find_package(asio REQUIRED)

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
    asio::asio
)

file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/libs.txt" 
     CONTENT "$<TARGET_PROPERTY:ai_srv,LINK_LIBRARIES>")
