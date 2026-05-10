# Architecture Overview

This document describes the architecture of the `ai_txt-system`.

## Bounded Context Diagram

```mermaid
C4Context
    title Bounded Context Diagram for ai_txt-system
    
    Person(user, "User", "Uses the CLI or Server to generate text.")
    System_Boundary(ai_system, "AI TXT System") {
        System(ai_cli, "ai_cli", "CLI for interacting with LLMs.")
        System(ai_srv, "ai_srv", "Microservice for interacting with LLMs.")
    }
    
    System_Ext(ollama, "Ollama API", "Local/External LLM Provider.")
    System_Ext(openrouter, "OpenRouter API", "Aggregator LLM Provider.")
    System_Ext(groq, "Groq API", "High-performance LLM Provider.")
    System_Ext(nvidia, "NVIDIA API", "Enterprise LLM Provider.")
    
    Rel(user, ai_cli, "Uses")
    Rel(user, ai_srv, "Uses (HTTP)")
    Rel(ai_cli, ollama, "Requests completions")
    Rel(ai_cli, openrouter, "Requests completions")
    Rel(ai_cli, groq, "Requests completions")
    Rel(ai_cli, nvidia, "Requests completions")
    Rel(ai_srv, ollama, "Requests completions")
    Rel(ai_srv, openrouter, "Requests completions")
    Rel(ai_srv, groq, "Requests completions")
    Rel(ai_srv, nvidia, "Requests completions")
```

## Class Diagram

```mermaid
classDiagram
    class Task {
        <<enumeration>>
        General
        Coding
        Translation
    }

    class ILlmProvider {
        <<interface>>
        +send_prompt(prompt, model) expected
        +get_models() vector
        +get_type() ProviderType
        +get_name() string
    }
    
    class HttpLlmProvider {
        #endpoint: string
        #api_key: string
        #models: vector
        #post_json(payload) HttpResult
    }
    
    class OllamaProvider {
        +send_prompt(prompt, model)
    }
    
    class OpenRouterProvider {
        +send_prompt(prompt, model)
    }

    class GroqProvider {
        +send_prompt(prompt, model)
    }

    class NvidiaProvider {
        +send_prompt(prompt, model)
    }
    
    class ProviderManager {
        -providers: vector~unique_ptr~
        +add_provider(provider)
        +request(prompt, preferred) expected
        +classify_task(prompt) Task
    }
    
    ILlmProvider <|.. HttpLlmProvider
    HttpLlmProvider <|-- OllamaProvider
    HttpLlmProvider <|-- OpenRouterProvider
    HttpLlmProvider <|-- GroqProvider
    HttpLlmProvider <|-- NvidiaProvider
    ProviderManager o-- ILlmProvider
    ProviderManager ..> Task : uses
```

## Failover & Scoring Sequence

```mermaid
sequenceDiagram
    participant C as Client
    participant PM as ProviderManager
    participant P as Providers (Ollama, OR, Groq, Nvidia)
    
    C->>PM: request(prompt, preferred)
    PM->>PM: classify_task(prompt)
    PM->>PM: Score all models (Task + Provider + Preference)
    PM->>PM: Sort Candidates (Highest Score First)
    
    loop For each Candidate
        PM->>P: send_prompt(prompt, model)
        alt Success
            P-->>PM: LlmResponse
            PM-->>C: JSON Response
        else Failure
            P-->>PM: error
            Note over PM: Try next candidate in sorted list
        end
    end
```
