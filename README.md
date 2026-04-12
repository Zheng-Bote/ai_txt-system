# ai_txt-system

Consolidated AI text generation system with Ollama and OpenRouter support.

## Documentation Overview

This project provides a consolidated interface for interacting with various LLM providers.
- **`ai_cli`**: Command-line interface for simple prompts and provider selection.
- **`ai_srv`**: Microservice using Crow to provide a REST API for LLM interaction.

The system handles multiple models per provider and includes failover logic to switch between models and providers if errors occur.

## Architecture Overview

The system is built on a provider-based architecture:
- `ILlmProvider`: Interface for all LLM services.
- `OllamaProvider`: Implementation for Ollama API.
- `OpenRouterProvider`: Implementation for OpenRouter API.
- `ProviderManager`: Orchestrates requests, handles preferred provider selection, and implements failover logic.

```mermaid
graph TD
    A[Client] -->|Prompt| B[ai_cli / ai_srv]
    B --> C[ProviderManager]
    C --> D[OllamaProvider]
    C --> E[OpenRouterProvider]
    D -->|Failover| E
    E -->|Failover| D
```

## Build

This project uses the [Conan](https://conan.io/) package manager.

```bash
# 1. Install dependencies
conan install . --output-folder=build --build=missing

# 2. Configure and build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=build/conan_toolchain.cmake
cmake --build build -j"$(nproc)"
```

## Docker deployment

You can easily deploy `ai_srv` via Docker using the pre-built binary. The `data/Dockerfile` provides the build instructions.

```bash
# Build the image
docker build -t ai_txt -f data/Dockerfile .

# Run the container with a mounted configuration directory
docker run -d \
  --name ai_txt \
  -p 18080:18080 \
  -v /home/Docker/webapp/etc/ai:/webapp/etc/ai \
  ai_txt
```

## Usage

### CLI (`ai_cli`)

The CLI tool allows sending prompts directly to configured LLM providers.

```bash
./build/ai_cli [OPTIONS] <PROMPT>
```

**Options:**
- `--provider <ollama|openrouter>`: Forces the use of a specific provider. If omitted, the system tries all providers in order (failover).
- `--env <path>`: Specify a custom path to the environment file (default: `data/private.env`).
- `--help, -h`: Show usage information.

**Examples:**
```bash
# Direct prompt
./build/ai_cli --provider ollama "What is C++23?"

# Using stdin
echo "Translate 'Hello' to German" | ./build/ai_cli
```

### Server (`ai_srv`)

The microservice provides a REST API on port `18080`.

```bash
# Basic startup
./build/ai_srv

# With custom environment file
./build/ai_srv --env /path/to/private.env
```

**Endpoint:** `POST /api/v1/prompt`

**Request Body (JSON):**
- `prompt` (string): The text to process.
- `provider` (string, optional): 'ollama' or 'openrouter'.

**Headers:**
- `X-LLM-Provider`: (optional) 'ollama' or 'openrouter'. Takes precedence over JSON body.

**Example:**
```bash
curl -X POST http://localhost:18080/api/v1/prompt \
     -H "Content-Type: application/json" \
     -H "X-LLM-Provider: openrouter" \
     -d '{"prompt": "Tell me a joke"}'
```
