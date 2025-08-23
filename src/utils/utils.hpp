#pragma once
#include <wups.h>

#include <coreinit/title.h>
#include <coreinit/mcp.h>

#include <cstring>
#include <string>
#include <vector>

namespace utils {
    bool isMiiverseClientID(const char *client_id);
    bool isMiiverseTitleID(uint32_t title_id);

    MCPSystemVersion getSystemVersion();

    char getConsoleRegion();

    bool is555OrHigher();
}; // namespace utils