# Validador de CPF — Bot do Telegram

![C++](https://img.shields.io/badge/C%2B%2B-17-00599C?style=flat&logo=cplusplus&logoColor=white)

Bot de Telegram escrito em **C++** que valida números de CPF brasileiros usando o algoritmo oficial dos dígitos verificadores.

## Sobre

O bot escuta mensagens via *long polling* da API do Telegram, identifica se o texto recebido se parece com um CPF e responde dizendo se ele é válido ou não. Aceita CPFs com ou sem formatação e rejeita os casos com todos os dígitos iguais (`111.111.111-11`, por exemplo).

## Funcionalidades

- Comando `/start` com instruções de uso
- Validação completa pelos dois dígitos verificadores
- Aceita CPF com ou sem formatação (`529.982.247-25` ou `52998224725`)
- Rejeita CPFs com todos os dígitos iguais
- Token carregado a partir de arquivo `.env` (nunca exposto no código)

## Tecnologias

C++17 · libcurl · JsonCpp · Telegram Bot API

## Pré-requisitos

- Linux (Ubuntu/Mint) ou WSL no Windows
- Token gerado pelo [@BotFather](https://t.me/BotFather)

Instalando as bibliotecas:

```bash
sudo apt install build-essential libcurl4-openssl-dev libjsoncpp-dev
```

## Como rodar

Clone o repositório e crie o arquivo `.env` na raiz com seu token:

```bash
git clone https://github.com/HugoSants/<nome-do-repo>.git
cd <nome-do-repo>
echo "BOT_TOKEN=seu_token_aqui" > .env
```

Compile e execute:

```bash
g++ -std=c++17 main.cpp -o cpf-bot -lcurl -ljsoncpp
./cpf-bot
```

Quando aparecer `Bot iniciado. Aguardando mensagens...`, é só abrir o Telegram e conversar com o bot.

## Estrutura

```
.
├── main.cpp     # código-fonte do bot
├── .env         # token (não versionado)
├── .gitignore
└── README.md
```

> O `.env` está listado no `.gitignore` e nunca deve ser enviado ao repositório.

## Exemplos de uso

| Entrada | Resposta |
|---|---|
| `/start` | mensagem de boas-vindas |
| `529.982.247-25` | ✅ CPF válido |
| `52998224725` | ✅ CPF válido |
| `111.111.111-11` | ❌ CPF inválido |
| `abc` | "não reconheci como CPF" |

## Roadmap

- [ ] Makefile para automatizar a compilação
- [ ] Tratamento de erros de rede
- [ ] Logs em arquivo (não só `std::cout`)
- [ ] Testes unitários para `validarCPF`
- [ ] Deploy com `systemd` para rodar 24/7
- [ ] Containerização com Docker
