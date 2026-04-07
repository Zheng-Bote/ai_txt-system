# llm_provider.hpp

## File Header Information

| Field | Value |
| :--- | :--- |
| **SPDX Comment** | LLM Provider Interface and Implementations |
| **SPDX Type** | SOURCE |
| **Contributor** | ZHENG Robert |
| **License ID** | Apache-2.0 |
| **File** | `llm_provider.hpp` |
| **Description** | LLM Provider Interface and Implementations |
| **Version** | 0.1.0 |
| **Date** | 2026-04-07 |
| **Author** | ZHENG Robert (robert@hase-zheng.net) |
| **Copyright** | Copyright (c) 2026 ZHENG Robert |
| **License** | Apache-2.0 |

<!-- START doctoc generated TOC please keep comment here to allow auto update -->
<!-- END doctoc generated TOC please keep comment here to allow auto update -->

## API Documentation

### `enum class ProviderType `

> Enum representing the LLM provider types.

---

### `struct LlmResponse `

> Structure representing a successful LLM response.

---

### `class ILlmProvider `

> Interface for LLM providers.

---

### `[[nodiscard]] virtual std::expected<LlmResponse, std::string> send_prompt(const std::string& prompt, const std::string& model) = 0`

> Sends a prompt to the LLM.

| Parameter | Description |
| --- | --- |
| `prompt` | The prompt text. |
| `model` | The specific model to use. |

**Returns:** std::expected<LlmResponse, std::string> The response or an error message.

---

### `[[nodiscard]] virtual std::vector<std::string> get_models() const = 0`

> Gets the list of available models for this provider.

**Returns:** std::vector<std::string> List of model names.

---

### `[[nodiscard]] virtual ProviderType get_type() const = 0`

> Gets the type of the provider.

**Returns:** ProviderType The provider type.

---

### `[[nodiscard]] virtual std::string get_name() const = 0`

> Gets the name of the provider.

**Returns:** std::string The provider name.

---

### `class HttpLlmProvider : public ILlmProvider `

> Base class for HTTP-based LLM providers.

---

### `HttpResult post_json(const json& payload, int timeout_seconds = 30) const`

> Performs a POST request with JSON payload.

---

### `class OllamaProvider : public HttpLlmProvider `

> Ollama provider implementation.

---

### `class OpenRouterProvider : public HttpLlmProvider `

> OpenRouter provider implementation.

---

### `class ProviderManager `

> Manager to handle multiple providers and failover logic.

---

### `[[nodiscard]] std::expected<LlmResponse, std::string> request(const std::string& prompt, ProviderType preferred_provider = ProviderType::Any)`

> Requests a prompt completion with failover.

| Parameter | Description |
| --- | --- |
| `prompt` | The prompt text. |
| `preferred_provider` | Optional preferred provider. |

**Returns:** std::expected<LlmResponse, std::string> The response or an error message.

---

### `std::map<std::string, std::string> load_config(const std::string& env_path)`

> Utility to parse .env files and environment variables.

---

