/**
 * SPDX-FileComment: Consolidated AI Microservice implementation
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file ai_srv.cpp
 * @brief Consolidated AI Microservice implementation
 * @version 0.1.0
 * @date 2026-04-07
 *
 * @author ZHENG Robert (robert@hase-zheng.net)
 * @copyright Copyright (c) 2026 ZHENG Robert
 *
 * @license Apache-2.0
 */

#include "llm_provider.hpp"
#include <crow.h>
#include <print>
#include <regex>

using namespace ai_txt;

/**
 * @brief Helper to collect LLMs for a specific prefix
 */
std::vector<std::string> collect_models(const std::map<std::string, std::string>& config, const std::string& prefix) {
    std::vector<std::pair<int, std::string>> tmp;
    std::regex r(prefix + R"(0*([0-9]+)$)", std::regex::icase);
    for (const auto& [key, val] : config) {
        std::smatch m;
        if (std::regex_search(key, m, r)) {
            int idx = std::stoi(m[1].str());
            tmp.emplace_back(idx, val);
        }
    }
    std::sort(tmp.begin(), tmp.end(), [](auto& a, auto& b) { return a.first < b.first; });
    std::vector<std::string> out;
    for (auto& p : tmp) out.push_back(p.second);
    return out;
}

int main() {
    crow::SimpleApp app;

    std::string env_path = "data/private.env";
    auto config = load_config(env_path);

    // Prepare Manager (singleton-ish for the route)
    // In a real app we might use a dependency injection or a global.
    auto create_manager = [&config]() {
        auto manager = std::make_unique<ProviderManager>();
        
        // Setup Ollama
        {
            std::string endpoint = config.contains("OLLAMA_ENDPOINT") ? config["OLLAMA_ENDPOINT"] : "https://ollama.com/api/chat";
            std::string api_key = config.contains("OLLAMA_API_KEY") ? config["OLLAMA_API_KEY"] : "";
            auto models = collect_models(config, "OLLAMA_LLM_");
            if (models.empty()) models = {"gemma3:4b-cloud"};
            manager->add_provider(std::make_unique<OllamaProvider>(endpoint, api_key, models));
        }

        // Setup OpenRouter
        {
            std::string endpoint = config.contains("OPENROUTER_ENDPOINT") ? config["OPENROUTER_ENDPOINT"] : "https://openrouter.ai/api/v1/chat/completions";
            std::string api_key = config.contains("OPENROUTER_API_KEY") ? config["OPENROUTER_API_KEY"] : "";
            auto models = collect_models(config, "OPENROUTER_LLM_");
            if (models.empty()) models = {"gpt-4o-mini"};
            manager->add_provider(std::make_unique<OpenRouterProvider>(endpoint, api_key, models));
        }
        return manager;
    };

    auto manager = create_manager();

    CROW_ROUTE(app, "/api/v1/prompt")
        .methods(crow::HTTPMethod::POST)([&manager](const crow::request& req) {
            auto req_json = json::parse(req.body, nullptr, false);
            std::string prompt;
            ProviderType preferred = ProviderType::Any;

            // 1. Get Preferred Provider from Header
            std::string header_p = req.get_header_value("X-LLM-Provider");
            if (header_p == "ollama") preferred = ProviderType::Ollama;
            else if (header_p == "openrouter") preferred = ProviderType::OpenRouter;

            // 2. Parse Body
            if (req_json.is_discarded()) {
                prompt = req.body;
            } else {
                if (req_json.contains("prompt")) prompt = req_json["prompt"].get<std::string>();
                else if (req_json.contains("text")) prompt = req_json["text"].get<std::string>();
                else prompt = req.body;

                if (preferred == ProviderType::Any && req_json.contains("provider")) {
                    std::string body_p = req_json["provider"].get<std::string>();
                    if (body_p == "ollama") preferred = ProviderType::Ollama;
                    else if (body_p == "openrouter") preferred = ProviderType::OpenRouter;
                }
            }

            if (prompt.empty()) return crow::response(400, "Empty prompt");

            auto res = manager->request(prompt, preferred);

            if (res) {
                json out;
                out["status"] = "success";
                out["response"] = res->text;
                out["model"] = res->model;
                out["provider"] = res->provider == ProviderType::Ollama ? "ollama" : "openrouter";
                return crow::response(out.dump());
            } else {
                json err;
                err["status"] = "error";
                err["message"] = res.error();
                return crow::response(502, err.dump());
            }
        });

    std::println("Starting Consolidated AI Server on port 18080...");
    app.port(18080).multithreaded().run();

    return 0;
}
