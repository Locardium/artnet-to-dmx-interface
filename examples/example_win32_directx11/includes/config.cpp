#include "Config.h"
#include <fstream>
#include <map>
#include <sstream>

const char* filePath = "C:\\Windows\\temp\\locos-artnet-to-dmx-config.ini";

namespace Config
{
    static bool loadAll(std::map<std::string, std::string>& outMap)
    {
        std::ifstream file(filePath);
        if (!file.is_open())
            return false;

        std::string line;
        while (std::getline(file, line))
        {
            if (line.empty() || line[0] == '#' || line[0] == ';')
                continue;

            auto pos = line.find('=');
            if (pos == std::string::npos)
                continue;

            std::string key = line.substr(0, pos);
            std::string val = line.substr(pos + 1);
            outMap[key] = val;
        }
        file.close();
        return true;
    }

    static bool saveAll(const std::map<std::string, std::string>& data)
    {
        std::ofstream file(filePath, std::ios::trunc);
        if (!file.is_open())
            return false;

        for (const auto& kv : data)
            file << kv.first << "=" << kv.second << "\n";

        file.close();
        return true;
    }

    std::string read(const std::string& key)
    {
        std::map<std::string, std::string> allConfig;
        if (!loadAll(allConfig))
            return "";

        auto it = allConfig.find(key);
        if (it == allConfig.end())
            return "";

        return it->second;
    }

    bool write(const std::string& key, const std::string& value)
    {
        std::map<std::string, std::string> allConfig;
        loadAll(allConfig);
        allConfig[key] = value;
        return saveAll(allConfig);
    }
}
