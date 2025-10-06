#pragma once

#include <string>
#include <vector>

namespace netInterface {
    std::vector<std::string> listInterfaces();
    bool create();
}
