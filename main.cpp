#include <iostream>
#include <string>
#include <curl/curl.h>
#include <json/json.h>
#include <unistd.h>

//podem proteger a variavel com .env
const std::string BOT_TOKEN = "8596323290:AAFN8JHW3BGTUbH7WnZKYIKuP_0jpkBzx48";

size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// Função genérica para fazer requisição HTTP GET
std::string fazerRequisicao(const std::string& url) {
    CURL* curl = curl_easy_init();
    std::string readBuffer;
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        // SEM CURLOPT_TIMEOUT para evitar interrupção precoce
        curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }
    return readBuffer;
}

// Valida CPF (inclui rejeição de dígitos repetidos)
bool validarCPF(const std::string& cpf) {
    std::string numeros;
    for (char c : cpf) {
        if (isdigit(c)) numeros += c;
    }
    if (numeros.length() != 11) return false;

    // Rejeitar CPFs com todos dígitos iguais
    bool todosIguais = true;
    for (size_t i = 1; i < numeros.length(); ++i) {
        if (numeros[i] != numeros[0]) {
            todosIguais = false;
            break;
        }
    }
    if (todosIguais) return false;

    // Primeiro dígito verificador
    int soma = 0;
    for (int i = 0; i < 9; i++) {
        soma += (numeros[i] - '0') * (10 - i);
    }
    int resto = soma % 11;
    int digito1 = (resto < 2) ? 0 : (11 - resto);
    if (digito1 != (numeros[9] - '0')) return false;

    // Segundo dígito verificador
    soma = 0;
    for (int i = 0; i < 10; i++) {
        soma += (numeros[i] - '0') * (11 - i);
    }
    resto = soma % 11;
    int digito2 = (resto < 2) ? 0 : (11 - resto);
    return (digito2 == (numeros[10] - '0'));
}

// Envia mensagem de volta para o usuário
void enviarMensagem(const std::string& chat_id, const std::string& texto) {
    CURL* curl = curl_easy_init();
    if (curl) {
        std::string texto_codificado = curl_easy_escape(curl, texto.c_str(), 0);
        std::string url = "https://api.telegram.org/bot" + BOT_TOKEN +
                          "/sendMessage?chat_id=" + chat_id +
                          "&text=" + texto_codificado;
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }
}

// Função auxiliar: verifica se a mensagem contém pelo menos 9 dígitos (pode ser um CPF)
bool pareceCPF(const std::string& texto) {
    int digitos = 0;
    for (char c : texto) {
        if (isdigit(c)) digitos++;
    }
    return digitos >= 9;
}

int main() {
    std::cout << "Bot iniciado. Aguardando mensagens...\n";
    long long last_update_id = 0;

    while (true) {
        std::string url_get = "https://api.telegram.org/bot" + BOT_TOKEN +
                              "/getUpdates?offset=" + std::to_string(last_update_id + 1) +
                              "&timeout=30";
        std::string resposta = fazerRequisicao(url_get);

        Json::Value root;
        Json::Reader reader;
        if (reader.parse(resposta, root) && root["ok"].asBool()) {
            for (const auto& update : root["result"]) {
                last_update_id = update["update_id"].asInt64();

                if (update.isMember("message") && update["message"].isMember("text")) {
                    std::string chat_id = std::to_string(update["message"]["chat"]["id"].asInt64());
                    std::string texto_usuario = update["message"]["text"].asString();

                    // Comando /start
                    if (texto_usuario == "/start") {
                        enviarMensagem(chat_id, "Olá! Envie um CPF (apenas números ou com pontos/traços) que eu valido.\nExemplo: 529.982.247-25");
                        continue;
                    }

                    // Filtro básico: precisa ter pelo menos 9 dígitos
                    if (!pareceCPF(texto_usuario)) {
                        enviarMensagem(chat_id, "Não reconheci isso como CPF. Envie um número com 11 dígitos.");
                        continue;
                    }

                    // Validação
                    if (validarCPF(texto_usuario)) {
                        enviarMensagem(chat_id, "✅ CPF " + texto_usuario + " é VÁLIDO!");
                    } else {
                        enviarMensagem(chat_id, "❌ CPF inválido. Verifique os dígitos.\nExemplo válido: 529.982.247-25");
                    }
                }
            }
        }
        sleep(2);
    }
    return 0;
}