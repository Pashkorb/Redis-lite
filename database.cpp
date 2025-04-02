#include "database.hpp"

void Database::set(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(mtx);
    data[key] = value;
}

std::string Database::get(const std::string& key) {
    std::lock_guard<std::mutex> lock(mtx);
    return data.count(key) ? data[key] : "(nil)";
}


bool Database::del(const std::string& key){
    std::lock_guard<std::mutex> lock(mtx);
    return data.erase(key)>0;

}
bool Database::exists(const std::string& key){
    std::lock_guard<std::mutex> lock(mtx);
    return data.count(key)>0;
}