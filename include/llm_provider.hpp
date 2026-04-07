/**
 * SPDX-FileComment: LLM Provider Interface and Implementations
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file llm_provider.hpp
 * @brief LLM Provider Interface and Implementations
 * @version 0.1.0
 * @date 2026-04-07
 *
 * @author ZHENG Robert (robert@hase-zheng.net)
 * @copyright Copyright (c) 2026 ZHENG Robert
 *
 * @license Apache-2.0
 */

#ifndef AI_TXT_SYSTEM_LLM_PROVIDER_HPP
#define AI_TXT_SYSTEM_LLM_PROVIDER_HPP

#include <expected>
#include <map>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace ai_txt {

using json = nlohmann::json;

/**
 * @brief Enum representing the LLM provider types.
 */
enum class ProviderType {
    Ollama,
    OpenRouter,
    Any
};

/**
 * @brief Structure representing a successful LLM response.
 */
struct LlmResponse {
    std::string text;
    std::string model;
    ProviderType provider;
};

/**
 * @brief Interface for LLM providers.
 */
class ILlmProvider {
public:
    virtual ~ILlmProvider() = default;

    /**
     * @brief Sends a prompt to the LLM.
     * 
     * @param prompt The prompt text.
     * @param model The specific model to use.
     * @return std::expected<LlmResponse, std::string> The response or an error message.
     */
    [[nodiscard]] virtual std::expected<LlmResponse, std::string> send_prompt(const std::string& prompt, const std::string& model) = 0;

    /**
     * @brief Gets the list of available models for this provider.
     * 
     * @return std::vector<std::string> List of model names.
     */
    [[nodiscard]] virtual std::vector<std::string> get_models() const = 0;

    /**
     * @brief Gets the type of the provider.
     * 
     * @return ProviderType The provider type.
     */
    [[nodiscard]] virtual ProviderType get_type() const = 0;

    /**
     * @brief Gets the name of the provider.
     * 
     * @return std::string The provider name.
     */
    [[nodiscard]] virtual std::string get_name() const = 0;
};

/**
 * @brief Base class for HTTP-based LLM providers.
 */
class HttpLlmProvider : public ILlmProvider {
public:
    HttpLlmProvider(std::string endpoint, std::string api_key, std::vector<std::string> models)
        : endpoint_(std::move(endpoint)), api_key_(std::move(api_key)), models_(std::move(models)) {}

    [[nodiscard]] std::vector<std::string> get_models() const override { return models_; }

protected:
    std::string endpoint_;
    std::string api_key_;
    std::vector<std::string> models_;

    struct HttpResult {
        long status = 0;
        std::string body;
        std::string error;
    };

    /**
     * @brief Performs a POST request with JSON payload.
     */
    HttpResult post_json(const json& payload, int timeout_seconds = 30) const;
};

/**
 * @brief Ollama provider implementation.
 */
class OllamaProvider : public HttpLlmProvider {
public:
    using HttpLlmProvider::HttpLlmProvider;

    [[nodiscard]] std::expected<LlmResponse, std::string> send_prompt(const std::string& prompt, const std::string& model) override;
    [[nodiscard]] ProviderType get_type() const override { return ProviderType::Ollama; }
    [[nodiscard]] std::string get_name() const override { return "Ollama"; }
};

/**
 * @brief OpenRouter provider implementation.
 */
class OpenRouterProvider : public HttpLlmProvider {
public:
    using HttpLlmProvider::HttpLlmProvider;

    [[nodiscard]] std::expected<LlmResponse, std::string> send_prompt(const std::string& prompt, const std::string& model) override;
    [[nodiscard]] ProviderType get_type() const override { return ProviderType::OpenRouter; }
    [[nodiscard]] std::string get_name() const override { return "OpenRouter"; }
};

/**
 * @brief Manager to handle multiple providers and failover logic.
 */
class ProviderManager {
public:
    void add_provider(std::unique_ptr<ILlmProvider> provider);
    
    /**
     * @brief Requests a prompt completion with failover.
     * 
     * @param prompt The prompt text.
     * @param preferred_provider Optional preferred provider.
     * @return std::expected<LlmResponse, std::string> The response or an error message.
     */
    [[nodiscard]] std::expected<LlmResponse, std::string> request(const std::string& prompt, ProviderType preferred_provider = ProviderType::Any);

private:
    std::vector<std::unique_ptr<ILlmProvider>> providers_;
};

/**
 * @brief Utility to parse .env files and environment variables.
 */
std::map<std::string, std::string> load_config(const std::string& env_path);

} // namespace ai_txt

#endif // AI_TXT_SYSTEM_LLM_PROVIDER_HPP
