#ifndef RAWTCP_TO_MQTT_BRIDGE_TCP_CONTEXT_HPP
#define RAWTCP_TO_MQTT_BRIDGE_TCP_CONTEXT_HPP

#include "slip.hpp"
#include <memory>
#include <string>
#include <any>
#include <unordered_map>

class TcpContext {
public:
    template<typename T>
    void set(const std::string& key, T value) {
        data_[key] = std::move(value);
    }

    template<typename T>
    T get(const std::string& key) const {
        return std::any_cast<T>(data_.at(key));
    }

    template<typename T>
    T* get_if(const std::string& key) {
        auto it = data_.find(key);
        if (it != data_.end()) {
            try {
                return std::any_cast<T>(&it->second);
            } catch (const std::bad_any_cast&) {
                return nullptr;
            }
        }
        return nullptr;
    }

    bool has(const std::string& key) const {
        return data_.contains(key);
    }

    void remove(const std::string& key) {
        data_.erase(key);
    }

private:
    std::unordered_map<std::string, std::any> data_;
};

#endif // RAWTCP_TO_MQTT_BRIDGE_TCP_CONTEXT_HPP
