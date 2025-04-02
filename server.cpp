#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include "database.hpp"
#include <atomic> 
#include <csignal>
#include <iostream>
#include <thread>
#include <vector>
#include <algorithm> 
#include <sstream> 

#define RESPONSE_END "\r\n"
#define _GLIBCXX_USE_CXX20_ABI 1
#define MAX_CMD_SIZE 1024
#define OK "OK"
#define NOTFOUND "NOT FOUND"
#define FOUND "FOUND"
#define ERROR "ERROR"
#define CLOSE_CLIENT close(client_fd)
#define OK_LEN 2
#define NOTFOUND_LEN 9
#define FOUND_LEN 5
#define ERROR_LEN 5

using std::string;

std::vector<std::thread> workers;
std::atomic<bool> server_running{true};

//выброс сигналов
void handler(int signal_num, siginfo_t* info, void* context) {
    std::cout << "Сигнал: " << signal_num 
              << " от PID: " << info->si_pid << "\n";
}

//регистрируем сигналы
void regestration_signal(){
    struct sigaction sa;
    sa.sa_sigaction = handler;  // Расширенный обработчик
    sa.sa_flags = SA_SIGINFO;   // Чтобы получать доп. информацию

    sigaction(SIGINT, &sa, nullptr);  // Ctrl+C
    sigaction(SIGTERM, &sa, nullptr); // kill
    sigaction(SIGSEGV, &sa, nullptr); // Segmentation fault

}
//вынос обработки клиента 
void handle_client(int client_fd, Database& db) {
    char buffer[MAX_CMD_SIZE];
    
    while (true) {
        ssize_t bytes = read(client_fd, buffer, sizeof(buffer) - 1);
        if (bytes <= 0) {
            CLOSE_CLIENT;
            return;
        }
        buffer[bytes] = '\0';
        std::string cmd(buffer);

        cmd.erase(std::remove(cmd.begin(), cmd.end(), '\r'), cmd.end());
        cmd.erase(std::remove(cmd.begin(), cmd.end(), '\n'), cmd.end());

        std::vector<std::string> tokens;
        std::istringstream iss(cmd);
        std::string token;
        while (iss >> token) {
            tokens.push_back(token);
        }

        if (tokens.empty()) {
            std::string response = "ERROR: empty command" + std::string(RESPONSE_END);
            write(client_fd, response.c_str(), response.size());
            continue;
        }

        const std::string& command = tokens[0];

        if (command == "SET" && tokens.size() >= 3) {
            db.set(tokens[1], tokens[2]);
            std::string response = std::string(OK) + RESPONSE_END;
            write(client_fd, response.c_str(), response.size());
        }
        else if (command == "GET" && tokens.size() >= 2) {
            std::string response = db.get(tokens[1]) + RESPONSE_END;
            write(client_fd, response.c_str(), response.size());
        }
        else if (command == "DEL" && tokens.size() >= 2) {
            bool deleted = db.del(tokens[1]);
            std::string response = std::string(deleted ? OK : NOTFOUND) + RESPONSE_END;
            write(client_fd, response.c_str(), response.size());
        }
        else if (command == "EXIST" && tokens.size() >= 2) {
            bool exists = db.exists(tokens[1]);
            std::string response = std::string(exists ? FOUND : NOTFOUND) + RESPONSE_END;
            write(client_fd, response.c_str(), response.size());
        }
        else {
            std::string response = "ERROR: invalid command" + std::string(RESPONSE_END);
            write(client_fd, response.c_str(), static_cast<size_t>(response.size()));
        }
    }
}
//основная функция
int main(){

    regestration_signal();//регистрация сигналов

    Database db; //ядро базы данных
    int server_fd=socket(AF_INET, SOCK_STREAM,0);//создаем сокет IPv4, TCP, 0 (протокол по умолчанию) server_fd - дескриптор
    sockaddr_in addr= {AF_INET,htons(6379),INADDR_ANY}; //настраиваем адресс сервера - IPv4, перевод в сетевой порядок номера порта, слушаем все интерфейсы
    bind(server_fd,(sockaddr*)&addr, sizeof(addr));//привязка сокета к адресу
    listen(server_fd,5);//начало прослушивания

    //основной цикл
    while(server_running){
        int client_fd=accept(server_fd,nullptr,nullptr);
        workers.emplace_back(handle_client, client_fd, std::ref(db));
    }
    
    for (auto& t : workers) {
        if (t.joinable()) t.join();  // Ждём завершения всех потоков
    }
    close(server_fd);
    return 0;
}



