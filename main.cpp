
#include <iostream>
#include <string>
#include <curl/curl.h>
#include <json/json.h>
#include <unistd.h>
#include <fstream>
#include <map>
#include <cctype>

std::string BOT_TOKEN;

// Função usada pelo CURL para salvar resposta da API
size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {

    ((std::string*)userp)->append((char*)contents, size * nmemb);

    return size * nmemb;
}

// Faz requisição HTTP GET
std::string fazerRequisicao(const std::string& url) {

    CURL* curl = curl_easy_init();

    std::string readBuffer;

    if (curl) {

        // Define URL da requisição
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

        // Salva resposta da API
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);

        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        // Configurações SSL
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);

        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

        // Executa requisição
        curl_easy_perform(curl);

        // Libera memória
        curl_easy_cleanup(curl);
    }

    return readBuffer;
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

        auto pos = linha.find('=');

        // Ignora linhas inválidas
        if (pos == std::string::npos)
            continue;

        // Salva chave e valor
        env[linha.substr(0, pos)] = linha.substr(pos + 1);
    }

    return env;
}

// Valida CPF
bool validarCPF(const std::string& cpf) {

    std::string numeros;

    // Remove pontos e traços
    for (char c : cpf) {

        if (isdigit(c))
            numeros += c;
    }

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

    for (int i = 0; i < 9; i++) {

        soma += (numeros[i] - '0') * (10 - i);
    }

    int resto = soma % 11;

    int digito1 = (resto < 2) ? 0 : (11 - resto);

    if (digito1 != (numeros[9] - '0'))
        return false;

    // Segundo dígito verificador
    soma = 0;

    for (int i = 0; i < 10; i++) {

        soma += (numeros[i] - '0') * (11 - i);
    }

    resto = soma % 11;

    int digito2 = (resto < 2) ? 0 : (11 - resto);

    return digito2 == (numeros[10] - '0');
}

// Envia mensagem para o Telegram
void enviarMensagem(const std::string& chat_id, const std::string& texto) {

    CURL* curl = curl_easy_init();

    if (curl) {

        // Codifica caracteres especiais
        std::string texto_codificado =
            curl_easy_escape(curl, texto.c_str(), 0);

        // Monta URL da API
        std::string url =
            "https://api.telegram.org/bot" + BOT_TOKEN +
            "/sendMessage?chat_id=" + chat_id +
            "&text=" + texto_codificado;

        // Define URL da requisição
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

        // Configurações SSL
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);

        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

        // Executa requisição
        curl_easy_perform(curl);

        // Libera memória
        curl_easy_cleanup(curl);
    }
}

// Verifica se possui 11 dígitos
bool pareceCPF(const std::string& texto) {

    int digitos = 0;

    for (char c : texto) {

        if (isdigit(c))
            digitos++;
    }

    return digitos == 11;
}

int main() {

    // Carrega variáveis do .env
    auto env = carregarEnv();

    BOT_TOKEN = env["BOT_TOKEN"];

    // Verifica se token existe
    if (BOT_TOKEN.empty()) {

        std::cerr << "Erro: BOT_TOKEN não encontrado no .env\n";

        return 1;
    }

    std::cout << "Bot iniciado. Aguardando mensagens...\n";

    long long last_update_id = 0;

    while (true) {

        // URL para buscar mensagens
        std::string url_get =
            "https://api.telegram.org/bot" + BOT_TOKEN +
            "/getUpdates?offset=" +
            std::to_string(last_update_id + 1) +
            "&timeout=30";

        std::string resposta = fazerRequisicao(url_get);

        Json::Value root;

        Json::Reader reader;

        // Verifica resposta da API
        if (reader.parse(resposta, root) &&
            root["ok"].asBool()) {

            for (const auto& update : root["result"]) {

                last_update_id =
                    update["update_id"].asInt64();

                // Verifica se recebeu texto
                if (update.isMember("message") &&
                    update["message"].isMember("text")) {

                    std::string chat_id =
                        std::to_string(
                            update["message"]["chat"]["id"].asInt64()
                        );

                    std::string texto_usuario =
                        update["message"]["text"].asString();

                    // Comando inicial
                    if (texto_usuario == "/start") {

                        enviarMensagem(
                            chat_id,
                            "Olá! Envie um CPF para validação.\nExemplo: 529.982.247-25"
                        );

                        continue;
                    }

                    // Verifica se parece CPF
                    if (!pareceCPF(texto_usuario)) {

                        enviarMensagem(
                            chat_id,
                            "Envie um CPF com 11 dígitos."
                        );

                        continue;
                    }

                    // Validação do CPF
                    if (validarCPF(texto_usuario)) {

                        enviarMensagem(
                            chat_id,
                            "✅ CPF " + texto_usuario + " é VÁLIDO!"
                        );

                    } else {

                        enviarMensagem(
                            chat_id,
                            "❌ CPF inválido. Verifique os dígitos."
                        );
                    }
                }
            }
        }

        sleep(2);
    }

    return 0;
}
