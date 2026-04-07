/**
 * SPDX-FileComment: Implementation of LLM Provider Interface
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file llm_provider.cpp
 * @brief Implementation of LLM Provider Interface
 * @version 0.1.0
 * @date 2026-04-07
 *
 * @author ZHENG Robert (robert@hase-zheng.net)
 * @copyright Copyright (c) 2026 ZHENG Robert
 *
 * @license Apache-2.0
 */

#include "llm_provider.hpp"

#include <curl/curl.h>
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <optional>
#include <print>
#include <regex>
#include <thread>

extern char** environ;

namespace ai_txt {

static size_t write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t total = size * nmemb;
    auto* s = static_cast<std::string*>(userp);
    s->append(static_cast<char*>(contents), total);
    return total;
}

HttpLlmProvider::HttpResult HttpLlmProvider::post_json(const json& payload, int timeout_seconds) const {
    HttpResult res;
    CURL* curl = curl_easy_init();
    if (!curl) {
        res.error = "Failed to initialize curl";
        return res;
    }

    std::string body;
    std::string payload_str = payload.dump();

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    if (!api_key_.empty()) {
        std::string auth = "Authorization: Bearer " + api_key_;
        headers = curl_slist_append(headers, auth.c_str());
    }

    curl_easy_setopt(curl, CURLOPT_URL, endpoint_.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload_str.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &body);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, static_cast<long>(timeout_seconds));
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    CURLcode code = curl_easy_perform(curl);
    if (code != CURLE_OK) {
        res.error = curl_easy_strerror(code);
    } else {
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &res.status);
        res.body = std::move(body);
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return res;
}

static std::optional<std::string> extract_text(const json& j) {
    // OpenRouter / OpenAI format
    if (j.contains("choices") && j["choices"].is_array() && !j["choices"].empty()) {
        auto c0 = j["choices"][0];
        if (c0.contains("message") && c0["message"].contains("content")) {
            if (c0["message"]["content"].is_string())
                return c0["message"]["content"].get<std::string>();
        }
    }
    // Ollama direct format
    if (j.contains("message") && j["message"].contains("content")) {
        if (j["message"]["content"].is_string())
            return j["message"]["content"].get<std::string>();
    }
    if (j.contains("response") && j["response"].is_string()) {
        return j["response"].get<std::string>();
    }
    return std::nullopt;
}

std::expected<LlmResponse, std::string> OllamaProvider::send_prompt(const std::string& prompt, const std::string& model) {
    json payload;
    payload["model"] = model;
    payload["messages"] = json::array({{{"role", "user"}, {"content", prompt}}});
    payload["stream"] = false;

    auto r = post_json(payload);

    if (!r.error.empty()) return std::unexpected("Ollama HTTP Error: " + r.error);
    if (r.status != 200) return std::unexpected("Ollama HTTP Status: " + std::to_string(r.status));

    try {
        json resp = json::parse(r.body);
        auto text = extract_text(resp);
        if (text) return LlmResponse{*text, model, ProviderType::Ollama};
        return std::unexpected("Ollama response missing content");
    } catch (const std::exception& e) {
        return std::unexpected("Ollama JSON error: " + std::string(e.what()));
    }
}

std::expected<LlmResponse, std::string> OpenRouterProvider::send_prompt(const std::string& prompt, const std::string& model) {
    json payload;
    payload["model"] = model;
    payload["messages"] = json::array({{{"role", "user"}, {"content", prompt}}});
    payload["max_tokens"] = 1024;
    payload["temperature"] = 0.2;

    auto r = post_json(payload);

    if (!r.error.empty()) return std::unexpected("OpenRouter HTTP Error: " + r.error);
    if (r.status != 200) return std::unexpected("OpenRouter HTTP Status: " + std::to_string(r.status));

    try {
        json resp = json::parse(r.body);
        auto text = extract_text(resp);
        if (text) return LlmResponse{*text, model, ProviderType::OpenRouter};
        return std::unexpected("OpenRouter response missing content");
    } catch (const std::exception& e) {
        return std::unexpected("OpenRouter JSON error: " + std::string(e.what()));
    }
}

void ProviderManager::add_provider(std::unique_ptr<ILlmProvider> provider) {
    providers_.push_back(std::move(provider));
}

std::expected<LlmResponse, std::string> ProviderManager::request(const std::string& prompt, ProviderType preferred_provider) {
    std::vector<ILlmProvider*> active_providers;
    for (auto& p : providers_) {
        if (preferred_provider == ProviderType::Any || p->get_type() == preferred_provider) {
            active_providers.push_back(p.get());
        }
    }

    // If preferred provider not found, and we wanted Any, it means no providers added.
    // If we wanted specific and not found, we could fallback or error. 
    // Requirement says: "Wird nichts angegeben, erfolgt die Nutzung beliebig."
    // If specified and not found, we should probably error or try others? 
    // Usually, preferred means "use this if available".
    
    if (active_providers.empty()) {
        if (preferred_provider != ProviderType::Any) {
            // Try all if preferred not found? Or just error. 
            // Let's error if specifically requested but missing.
            return std::unexpected("Requested provider not available");
        }
        return std::unexpected("No LLM providers configured");
    }

    std::string errors;
    for (auto* p : active_providers) {
        for (const auto& model : p->get_models()) {
            std::println(std::cerr, "[INFO] Trying {} with model: {}", p->get_name(), model);
            auto res = p->send_prompt(prompt, model);
            if (res) return res;
            
            std::println(std::cerr, "[WARN] {} model {} failed: {}", p->get_name(), model, res.error());
            if (!errors.empty()) errors += "; ";
            errors += p->get_name() + "(" + model + "): " + res.error();
            
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
    }

    // If preferred provider failed all models, should we try the other provider?
    // "Sollte ein LLM-Provider ausfallen, wird auf den aderen gewechselt."
    if (preferred_provider != ProviderType::Any) {
        std::println(std::cerr, "[INFO] Preferred provider failed, trying others...");
        for (auto& p : providers_) {
            if (p->get_type() != preferred_provider) {
                for (const auto& model : p->get_models()) {
                    std::println(std::cerr, "[INFO] Trying {} with model: {}", p->get_name(), model);
                    auto res = p->send_prompt(prompt, model);
                    if (res) return res;
                    
                    std::println(std::cerr, "[WARN] {} model {} failed: {}", p->get_name(), model, res.error());
                    if (!errors.empty()) errors += "; ";
                    errors += p->get_name() + "(" + model + "): " + res.error();
                    
                    std::this_thread::sleep_for(std::chrono::milliseconds(200));
                }
            }
        }
    }

    return std::unexpected("All providers/models failed: " + errors);
}

std::map<std::string, std::string> load_config(const std::string& env_path) {
    std::map<std::string, std::string> out;
    std::ifstream ifs(env_path);
    if (ifs) {
        std::string line;
        std::regex kv(R"(^\s*([A-Za-z0-9_]+)\s*=\s*(.*)\s*$)");
        while (std::getline(ifs, line)) {
            auto pos = line.find('#');
            if (pos != std::string::npos) line = line.substr(0, pos);
            std::smatch m;
            if (std::regex_match(line, m, kv)) {
                std::string k = m[1].str();
                std::string v = m[2].str();
                if (v.size() >= 2 && ((v.front() == '"' && v.back() == '"') || (v.front() == '\'' && v.back() == '\''))) {
                    v = v.substr(1, v.size() - 2);
                }
                out[k] = v;
            }
        }
    }
    // Overlay process environment
    for (char** env = environ; *env; ++env) {
        std::string s = *env;
        auto pos = s.find('=');
        if (pos != std::string::npos) {
            std::string k = s.substr(0, pos);
            if (out.find(k) == out.end()) {
                out[k] = s.substr(pos + 1);
            }
        }
    }
    return out;
}

} // namespace ai_txt
