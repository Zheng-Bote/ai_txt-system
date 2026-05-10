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

std::expected<LlmResponse, std::string> GroqProvider::send_prompt(const std::string& prompt, const std::string& model) {
    json payload;
    payload["model"] = model;
    payload["messages"] = json::array({{{"role", "user"}, {"content", prompt}}});
    
    auto r = post_json(payload);

    if (!r.error.empty()) return std::unexpected("Groq HTTP Error: " + r.error);
    if (r.status != 200) return std::unexpected("Groq HTTP Status: " + std::to_string(r.status));

    try {
        json resp = json::parse(r.body);
        auto text = extract_text(resp);
        if (text) return LlmResponse{*text, model, ProviderType::Groq};
        return std::unexpected("Groq response missing content");
    } catch (const std::exception& e) {
        return std::unexpected("Groq JSON error: " + std::string(e.what()));
    }
}

std::expected<LlmResponse, std::string> NvidiaProvider::send_prompt(const std::string& prompt, const std::string& model) {
    json payload;
    payload["model"] = model;
    payload["messages"] = json::array({{{"role", "user"}, {"content", prompt}}});
    
    auto r = post_json(payload);

    if (!r.error.empty()) return std::unexpected("Nvidia HTTP Error: " + r.error);
    if (r.status != 200) return std::unexpected("Nvidia HTTP Status: " + std::to_string(r.status));

    try {
        json resp = json::parse(r.body);
        auto text = extract_text(resp);
        if (text) return LlmResponse{*text, model, ProviderType::Nvidia};
        return std::unexpected("Nvidia response missing content");
    } catch (const std::exception& e) {
        return std::unexpected("Nvidia JSON error: " + std::string(e.what()));
    }
}

void ProviderManager::add_provider(std::unique_ptr<ILlmProvider> provider) {
    providers_.push_back(std::move(provider));
}

Task ProviderManager::classify_task(const std::string& prompt) {
    std::string p = prompt;
    std::transform(p.begin(), p.end(), p.begin(), [](unsigned char c){ return std::tolower(c); });

    if (p.find("code") != std::string::npos || p.find("methode") != std::string::npos || 
        p.find("function") != std::string::npos || p.find("c++") != std::string::npos ||
        p.find("python") != std::string::npos || p.find("java") != std::string::npos) {
        return Task::Coding;
    }

    if (p.find("übersetze") != std::string::npos || p.find("translate") != std::string::npos ||
        p.find("nach de") != std::string::npos || p.find("to en") != std::string::npos) {
        return Task::Translation;
    }

    return Task::General;
}

std::expected<LlmResponse, std::string> ProviderManager::request(const std::string& prompt, ProviderType preferred_provider) {
    Task task = classify_task(prompt);
    std::string task_str = (task == Task::Coding) ? "Coding" : (task == Task::Translation ? "Translation" : "General");
    std::println(std::cerr, "[INFO] Detected task: {}", task_str);

    struct ModelCandidate {
        ILlmProvider* provider;
        std::string model;
        int score;
    };

    std::vector<ModelCandidate> candidates;

    for (auto& p : providers_) {
        for (const auto& model : p->get_models()) {
            int score = 0;
            std::string m_lower = model;
            std::transform(m_lower.begin(), m_lower.end(), m_lower.begin(), [](unsigned char c){ return std::tolower(c); });

            // Base score by provider
            if (p->get_type() == preferred_provider) score += 1000;
            
            // Task specific scoring
            if (task == Task::Coding) {
                if (m_lower.find("coder") != std::string::npos || m_lower.find("code") != std::string::npos) score += 500;
                if (m_lower.find("qwen") != std::string::npos || m_lower.find("deepseek") != std::string::npos) score += 200;
            } else if (task == Task::Translation) {
                if (m_lower.find("mistral") != std::string::npos || m_lower.find("gemini") != std::string::npos) score += 500;
            }

            // Provider priority if no preference
            if (preferred_provider == ProviderType::Any) {
                switch (p->get_type()) {
                    case ProviderType::Nvidia: score += 50; break;
                    case ProviderType::Groq: score += 40; break;
                    case ProviderType::OpenRouter: score += 30; break;
                    case ProviderType::Ollama: score += 20; break;
                    default: break;
                }
            }

            candidates.push_back({p.get(), model, score});
        }
    }

    if (candidates.empty()) return std::unexpected("No LLM models available");

    std::sort(candidates.begin(), candidates.end(), [](const auto& a, const auto& b) {
        return a.score > b.score;
    });

    std::string errors;
    for (const auto& c : candidates) {
        std::println(std::cerr, "[INFO] Trying {} with model: {} (Score: {})", c.provider->get_name(), c.model, c.score);
        auto res = c.provider->send_prompt(prompt, c.model);
        if (res) return res;

        std::println(std::cerr, "[WARN] {} model {} failed: {}", c.provider->get_name(), c.model, res.error());
        if (!errors.empty()) errors += "; ";
        errors += c.provider->get_name() + "(" + c.model + "): " + res.error();
        
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    return std::unexpected("All candidates failed: " + errors);
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
