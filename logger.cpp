#include "logger.hpp"
#include <filesystem>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <algorithm>

using std::vector;

namespace fs = std::filesystem;

Logger::~Logger() {
    running = false;
    flush(); 
    if (worker_thread.joinable()) {
        worker_thread.join();
    }
}

bool directoryExists(const std::string& path) {
    return fs::exists(path) && fs::is_directory(path);
}

std::string Logger::timestamp_to_string(
    const std::chrono::system_clock::time_point& tp) 
{
    auto time = std::chrono::system_clock::to_time_t(tp);
    std::tm tm = *std::localtime(&time);
    
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

void Logger::init(const std::string& log_dir) {
    this->log_dir = log_dir;
    if (!fs::exists(log_dir)) {
        fs::create_directories(log_dir);
    }
    worker_thread = std::thread(&Logger::logger_worker, this);
}

void Logger::flush() {
    std::unique_lock lock(buffer_mutex);
    cv.notify_one();        // Будим фоновый поток
    cv.wait(lock, [&] {     // Ждём завершения записи
        return buffer.empty(); 
    });
}

std::string Logger::level_to_string(Level level) {
    static const char* strings[] = {
        "UNKNOWN", "ERROR", "INFO", "WARNING", "DEBUG"
    };
    
    if (level < ERROR || level > DEBUG) {
        level = static_cast<Level>(0);
    }
    return strings[level];
}

void Logger::log(Level level, const std::string& message){
    auto timestamp = std::chrono::system_clock::now();
    std::string log_entry ="[" + timestamp_to_string(timestamp) + "] " + 
           level_to_string(level) + ": " + 
           message;

    std::lock_guard lock(buffer_mutex);
    buffer.push_back(log_entry);
    
    if(buffer.size()>= BUFFER_FLUSH_SIZE){
        cv.notify_one();
    }

}

void Logger::rotate_log() {
    // Закрываем текущий файл, если открыт
    if (current_file.is_open()) {
        current_file.close();
    }

    // Получаем список существующих лог-файлов
    std::vector<fs::directory_entry> log_files;
    for (const auto& entry : fs::directory_iterator(log_dir)) {
        if (entry.is_regular_file() && 
            entry.path().extension() == ".log" &&
            entry.path().filename().string().find("redis_") == 0) {
            log_files.push_back(entry);
        }
    }

    // Сортируем по дате изменения (новые в начале)
    std::sort(log_files.begin(), log_files.end(), 
        [](const auto& a, const auto& b) {
            return fs::last_write_time(a) > fs::last_write_time(b);
        });

    // Удаляем старые логи (оставляем последние 10)
    while (log_files.size() > 10) {
        fs::remove(log_files.back().path());
        log_files.pop_back();
    }

    // Создаём новый файл
    auto now = std::chrono::system_clock::now();
    std::time_t time = std::chrono::system_clock::to_time_t(now);
    std::tm tm = *std::localtime(&time);
    
    std::ostringstream filename;
    filename << log_dir << "/redis_"
             << std::put_time(&tm, "%Y-%m-%d_%H-%M-%S") << ".log";
    
    current_file.open(filename.str(), std::ios::app);
    current_size = 0;
    
    if (!current_file) {
        throw std::runtime_error("Cannot open log file: " + filename.str());
    }
}

void Logger::write_to_file(const std::deque<std::string>& messages){
    if(!current_file.is_open()){
        rotate_log();
    }
    for(const auto& message:messages){
        current_file <<message <<"\n";
        current_size+=message.size();
        if(current_size>=MAX_FILE_SIZE){
            rotate_log();
        }
    }
    current_file.flush();

}

void Logger::logger_worker() {
    while (running) {
        std::unique_lock lock(buffer_mutex);
        
        // Ждём либо таймаут, либо заполнения буфера
        cv.wait_for(lock, std::chrono::seconds(5), [&]{
            return buffer.size() >= BUFFER_FLUSH_SIZE || flush_requested;
        });
        
        // Копируем данные для записи
        auto data_to_write = std::move(buffer);
        buffer.clear();
        flush_requested = false;
        
        lock.unlock(); // Разблокируем прежде чем писать в файл!
        
        // Медленная запись (уже без блокировки)
        write_to_file(data_to_write);
    }
}

