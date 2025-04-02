#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include "database.hpp"
#include <atomic> 
#include <csignal>
#include <iostream>
#define _GLIBCXX_USE_CXX20_ABI 1

#define OK "OK"
#define OK_Len 2
#define NOTFOUND "NOT FOUND"
#define NOTFOUND_Len 9
#define FOUND "FOUND"
#define FOUND_Len 5
#define ERROR "ERROR"
#define ERROR_LEN 5
#define CLOSE_CLIENT close(client_fd)

using std::string;

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
        char buffer[1024];
        ssize_t bytes = read(client_fd, buffer, sizeof(buffer));
        if (bytes <= 0) { close(client_fd); continue; } //проверяем что ввод не пустой

        //парсим команду SET
        std::string cmd(buffer);
        if (cmd.rfind("SET ", 0) == 0){
            auto pos = cmd.find(' ', 4);
            if (pos == std::string::npos) { 
                write(client_fd, ERROR, 5); 
                CLOSE_CLIENT; 
                continue;
            }
            db.set(cmd.substr(4, pos - 4), cmd.substr(pos + 1));
            write(client_fd,OK,OK_Len);
            CLOSE_CLIENT;
            continue;
        }
        
        //Парсим команду GET
        if (cmd.rfind("GET ", 0) == 0){
            auto pos=cmd.find(' ',4);
            if (pos == std::string::npos) { 
                write(client_fd, ERROR, 5); 
                CLOSE_CLIENT; 
                continue;
            }
            string key = cmd.substr(4, pos - 4);
            string value = db.get(key);
            write(client_fd, value.c_str(), value.size()); 
            CLOSE_CLIENT;
            continue;
        }

        //Парсим команду DEL
        if (cmd.rfind("DEL ", 0) == 0){
            auto pos=cmd.find(' ',4);
            if (pos == std::string::npos) { 
                write(client_fd, ERROR, 5); 
                CLOSE_CLIENT; 
                continue;
            }
            string key = cmd.substr(4, pos - 4);
            bool delit = db.del(key);
            if(delit){
                write(client_fd, OK, OK_Len); 
                CLOSE_CLIENT;
                continue;
            }
            else{
                write(client_fd,NOTFOUND, NOTFOUND_Len); 
                CLOSE_CLIENT;
                continue;
            }

        }

        //Парсим команду EXIST
        if (cmd.rfind("EXIST ", 0) == 0){
            auto pos=cmd.find(' ',6);
            if (pos == std::string::npos) { 
                write(client_fd, ERROR, ERROR_LEN); 
                CLOSE_CLIENT; 
                continue;
            }
            string key = cmd.substr(6, pos - 6);
            bool exists = db.exists(key);
            if(exists){
                write(client_fd, FOUND, FOUND_Len); 
                CLOSE_CLIENT;
                continue;
            }
            else{
                write(client_fd,NOTFOUND, NOTFOUND_Len); 
                CLOSE_CLIENT;
                continue;
            }

        CLOSE_CLIENT;
        }
        
    }
    
    return 0;
}



