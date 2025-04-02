#pragma once
#include <unordered_map>
#include <string>

class Database {
    std::unordered_map<std::string, std::string> data;
public:
    void set(const std::string& key, const std::string& value);
    std::string get(const std::string& key);
    bool del(const std::string& key);
    bool exists(const std::string& key);
};