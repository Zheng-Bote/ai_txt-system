/**
 * SPDX-FileComment: Consolidated AI CLI implementation
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file ai_cli.cpp
 * @brief Consolidated AI CLI implementation
 * @version 0.1.0
 * @date 2026-04-07
 *
 * @author ZHENG Robert (robert@hase-zheng.net)
 * @copyright Copyright (c) 2026 ZHENG Robert
 *
 * @license Apache-2.0
 */

#include "llm_provider.hpp"
#include <iostream>
#include <print>
#include <regex>
#include <vector>

using namespace ai_txt;

/**
 * @brief Helper to collect LLMs for a specific prefix (e.g., OLLAMA_LLM_)
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

int main(int argc, char** argv) {
    std::string env_path = "data/private.env";
    ProviderType preferred = ProviderType::Any;
    std::string prompt;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--provider" && i + 1 < argc) {
            std::string p = argv[++i];
            if (p == "ollama") preferred = ProviderType::Ollama;
            else if (p == "openrouter") preferred = ProviderType::OpenRouter;
        } else if (arg == "--env" && i + 1 < argc) {
            env_path = argv[++i];
        } else {
            if (!prompt.empty()) prompt += " ";
            prompt += arg;
        }
    }

    if (prompt.empty()) {
        std::string line;
        while (std::getline(std::cin, line)) {
            prompt += line + "\n";
        }
    }

    if (prompt.empty()) {
        std::println(std::cerr, "Usage: ai_cli [--provider ollama|openrouter] [--env path] <prompt>");
        return 1;
    }

    auto config = load_config(env_path);
    ProviderManager manager;

    // Setup Ollama
    {
        std::string endpoint = config.contains("OLLAMA_ENDPOINT") ? config["OLLAMA_ENDPOINT"] : "https://ollama.com/api/chat";
        std::string api_key = config.contains("OLLAMA_API_KEY") ? config["OLLAMA_API_KEY"] : "";
        auto models = collect_models(config, "OLLAMA_LLM_");
        if (models.empty()) models = {"gemma3:4b-cloud"};
        manager.add_provider(std::make_unique<OllamaProvider>(endpoint, api_key, models));
    }

    // Setup OpenRouter
    {
        std::string endpoint = config.contains("OPENROUTER_ENDPOINT") ? config["OPENROUTER_ENDPOINT"] : "https://openrouter.ai/api/v1/chat/completions";
        std::string api_key = config.contains("OPENROUTER_API_KEY") ? config["OPENROUTER_API_KEY"] : "";
        auto models = collect_models(config, "OPENROUTER_LLM_");
        if (models.empty()) models = {"gpt-4o-mini"};
        manager.add_provider(std::make_unique<OpenRouterProvider>(endpoint, api_key, models));
    }

    auto res = manager.request(prompt, preferred);

    if (res) {
        std::println("{}", res->text);
        return 0;
    } else {
        std::println(std::cerr, "Error: {}", res.error());
        return 1;
    }
}
