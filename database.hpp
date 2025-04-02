#pragma once
#include <unordered_map>
#include <string>
#include <mutex>

class Database {
    std::unordered_map<std::string, std::string> data;
    std::mutex mtx;
public:
    void set(const std::string& key, const std::string& value);
    std::string get(const std::string& key);
    bool del(const std::string& key);
    bool exists(const std::string& key);
};