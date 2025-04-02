// logger.hpp
#pragma once
#include <string>
#include <fstream>
#include <mutex>
#include <deque>
#include <atomic>
#include <condition_variable>

class Logger {
public:
public:
    enum Level { 
        ERROR = 1, 
        INFO, 
        WARNING, 
        DEBUG 
    };
    
    void init(const std::string& log_dir);
    void log(Level level, const std::string& message);
    void flush();
    ~Logger();

private:
    void rotate_log();
    std::string level_to_string(Level level);
    std::string timestamp_to_string(
        const std::chrono::system_clock::time_point& tp) ;
    void write_to_file(const std::deque<std::string>& messages);
    void logger_worker();
    std::atomic<bool> flush_requested{false};
    std::ofstream current_file;
    std::deque<std::string> buffer;
    std::mutex buffer_mutex;
    std::condition_variable cv;
    std::atomic<bool> running{true};
    size_t current_size = 0;
    const size_t MAX_FILE_SIZE = 10 * 1024 * 1024;
    const size_t BUFFER_FLUSH_SIZE = 100;
    std::string log_dir;
    std::thread worker_thread;
};