# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.1.0] - 2026-04-07

### Added
- **Conan Integration**: Added `conanfile.txt` for dependency management (`libcurl`, `nlohmann_json`, `crowcpp-crow`, `asio`).
- **CLI Improvements**: Added `--help` and `-h` flags to `ai_cli`.
- **Better Documentation**: Detailed CLI parameter documentation in the help message and README.

### Changed
- **CMake**: Updated `CMakeLists.txt` to use Conan-generated configuration files and targets.
- **Dependency Versioning**: Fixed `asio` version to `1.29.0` to ensure compatibility with `crowcpp-crow`.

## [1.0.0] - 2026-04-07

### Added
- **Consolidation**: Merged separate Ollama and OpenRouter clients into a single system.
- **Provider Interface**: Introduced `ILlmProvider` and `HttpLlmProvider` for unified LLM interaction.
- **Provider Manager**: New `ProviderManager` class to orchestrate requests across multiple providers.
- **Failover Logic**: 
    - Automatic model-level failover (retries with next available model).
    - Automatic provider-level failover (switches provider if all models fail).
- **Consolidated CLI (`ai_cli`)**: Single tool with `--provider` and `--env` support.
- **Consolidated Server (`ai_srv`)**: Single Crow-based microservice with header-based (`X-LLM-Provider`) and JSON-based provider selection.
- **Modern C++**: Implementation using C++23 features like `std::expected` and `std::print`/`std::println`.
- **Architecture Docs**: Added Mermaid diagrams for Bounded Context, Class Hierarchy, and Failover Sequence.

### Fixed
- **Linker Issues**: Resolved `environ` reference errors by correctly declaring it at global scope.

### Removed
- Legacy files: `src/ollama_cli_env.cpp`, `src/ollama_srv.cpp`, `src/openrouter_cli_env.cpp`, `src/openrouter_srv.cpp`.

## [0.2.0] - 2026-04-07 (Legacy)

### Added
- Initial implementation of Crow-based microservices for Ollama and OpenRouter.

## [0.1.0] - 2026-04-04 (Legacy)

### Added
- Initial CLI implementations for Ollama and OpenRouter.
