#pragma once

#include <string>

namespace Config
{
    std::string read(const std::string& key);
    bool write(const std::string& key, const std::string& value);

    inline bool write(const std::string& key, int value)
    {
        return write(key, std::to_string(value));
    }

    inline bool write(const std::string& key, char value)
    {
        return write(key, std::string(1, value));
    }

    inline bool write(const std::string& key, bool value)
    {
        return write(key, value ? "true" : "false");
    }

    template<typename T>
    inline bool write(const std::string& key, const T& value)
    {
        return write(key, std::to_string(value));
    }
}
