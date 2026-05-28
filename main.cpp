#include <iostream>
#include <string>
#include <curl/curl.h>
#include <jsoncpp/json/json.h>
#include <fstream>
#include <map>
#include <thread>
#include <chrono>
#include <cctype>
#include <memory>

// Função usada pelo CURL para salvar resposta da API
size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// Faz requisição HTTP GET
bool fazerRequisicao(const std::string& url, std::string& resposta, std::string& erro) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        erro = "Falha ao inicializar o cURL";
        return false;
    }

    // Define URL da requisição
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    
    // Salva resposta da API
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resposta);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 35L);
    
    // Configurações SSL
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);

    // Executa requisição
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        erro = curl_easy_strerror(res);
    }

    // Libera memória
    curl_easy_cleanup(curl);
    return (res == CURLE_OK);
}

// Lê o arquivo .env
std::map<std::string, std::string> carregarEnv(const std::string& caminho = ".env") {
    std::map<std::string, std::string> env;
    std::ifstream arquivo(caminho);
    std::string linha;

    while (std::getline(arquivo, linha)) {
        // Ignora linhas vazias e comentários
        if (linha.empty() || linha[0] == '#')
            continue;

        auto pos  = linha.find('=');
        if (pos == std::string::npos)
            continue;

        // Salva chave e valor
        env[linha.substr(0, pos)] = linha.substr(pos + 1);
    }
    return env;
}

std::string extrairDigitos(const std::string& texto) {
    std::string numeros;
    
    // Remove pontos e traços
    for (char c : texto) {
        if (std::isdigit(static_cast<unsigned char>(c))) {
            numeros.push_back(c);
        }
    }
    return numeros;
}

// Valida CPF
bool validarCPF(const std::string& cpf) {
    std::string numeros = extrairDigitos(cpf);

    // CPF precisa ter 11 dígitos
    if (numeros.length() != 11)
        return false;

    // Verifica números repetidos
    bool todosIguais = true;
    for (size_t i = 1; i < numeros.length(); ++i) {
        if (numeros[i] != numeros[0]) {
            todosIguais = false;
            break;
        }
    }
    if (todosIguais)
        return false;

    // Primeiro dígito verificador
    int soma = 0;
    for (int i = 0; i < 9; ++i) {
        soma += (numeros[i] - '0') * (10 - i);
    }
    int resto = soma % 11;
    int digito1 = (resto < 2) ? 0 : (11 - resto);
    if (digito1 != (numeros[9] - '0'))
        return false;

    // Segundo dígito verificador
    soma = 0;
    for (int i = 0; i < 10; ++i) {
        soma += (numeros[i] - '0') * (11 - i);
    }
    resto = soma % 11;
    int digito2 = (resto < 2) ? 0 : (11 - resto);
    
    return (digito2 == (numeros[10] - '0'));
}

// Envia mensagem para o Telegram
void enviarMensagem(const std::string& botToken, const std::string& chat_id, const std::string& texto) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        std::cerr << "Falha ao inicializar o cURL para enviar mensagem\n";
        return;
    }

    // Codifica caracteres especiais
    char* textoCodificado = curl_easy_escape(curl, texto.c_str(), static_cast<int>(texto.size()));
    if (!textoCodificado) {
        std::cerr << "Falha ao codificar o texto da mensagem\n";
        curl_easy_cleanup(curl);
        return;
    }

    // Monta URL da API
    std::string url = "https://api.telegram.org/bot" + botToken +
                      "/sendMessage?chat_id=" + chat_id +
                      "&text=" + textoCodificado;

    // Define URL da requisição
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    
    // Configurações SSL
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);

    // Executa requisição
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        std::cerr << "Erro ao enviar mensagem: " << curl_easy_strerror(res) << "\n";
    }

    // Libera memória
    curl_free(textoCodificado);
    curl_easy_cleanup(curl);
}

// Verifica se possui 11 dígitos
bool pareceCPF(const std::string& texto) {
    int digitos = 0;
    for (char c : texto) {
        if (std::isdigit(static_cast<unsigned char>(c))) digitos++;
    }
    return digitos == 11;
}

int main() {
    curl_global_init(CURL_GLOBAL_DEFAULT);
    
    // Carrega variáveis do .env
    auto env = carregarEnv();
    auto tokenIt = env.find("BOT_TOKEN");

    // Verifica se token existe
    if (tokenIt == env.end() || tokenIt->second.empty()) {
        std::cerr << "Erro: BOT_TOKEN não encontrado no .env\n";
        curl_global_cleanup();
        return 1;
    }

    const std::string botToken = tokenIt->second;
    std::cout << "Bot iniciado. Aguardando mensagens...\n";
    
    long long last_update_id = 0;

    while (true) {
        std::string resposta;
        std::string erro;
        
        // URL para buscar mensagens
        std::string url_get = "https://api.telegram.org/bot" + botToken +
                              "/getUpdates?offset=" + std::to_string(last_update_id + 1) +
                              "&timeout=30";

        if (!fazerRequisicao(url_get, resposta, erro)) {
            std::cerr << "Erro na requisição getUpdates: " << erro << "\n";
            std::this_thread::sleep_for(std::chrono::seconds(2));
            continue;
        }

        Json::Value root;
        Json::CharReaderBuilder readerBuilder;
        std::string parseErrors;
        std::unique_ptr<Json::CharReader> reader(readerBuilder.newCharReader());

        // Verifica resposta da API
        if (!reader->parse(resposta.c_str(), resposta.c_str() + resposta.size(), &root, &parseErrors)) {
            std::cerr << "Falha ao analisar JSON: " << parseErrors << "\n";
            std::this_thread::sleep_for(std::chrono::seconds(2));
            continue;
        }

        if (!root["ok"].asBool()) {
            std::cerr << "Resposta da API sem ok=true\n";
            std::this_thread::sleep_for(std::chrono::seconds(2));
            continue;
        }

        // Blindagem contra JSON corrompido ou malformado
        if (!root.isMember("result") || !root["result"].isArray()) {
            std::this_thread::sleep_for(std::chrono::seconds(2));
            continue;
        }

        for (const auto& update : root["result"]) {
            last_update_id = update["update_id"].asInt64();

            // Verifica se recebeu texto
            if (update.isMember("message") && update["message"].isMember("text")) {
                std::string chat_id = std::to_string(update["message"]["chat"]["id"].asInt64());
                std::string texto_usuario = update["message"]["text"].asString();

                // Comando inicial
                if (texto_usuario == "/start") {
                    enviarMensagem(botToken, chat_id, "Olá! Envie um CPF para validação.\nExemplo: 529.982.247-25");
                    continue;
                }

                // Verifica se parece CPF
                if (!pareceCPF(texto_usuario)) {
                    enviarMensagem(botToken, chat_id, "Envie um CPF com 11 dígitos.");
                    continue;
                }

                // Validação do CPF
                if (validarCPF(texto_usuario)) {
                    enviarMensagem(botToken, chat_id, "✅ CPF " + texto_usuario + " é VÁLIDO!");
                } else {
                    enviarMensagem(botToken, chat_id, "❌ CPF inválido. Verifique os dígitos.");
                }
            }
        }
        
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
    
    curl_global_cleanup();
    return 0;
}
