#include <wups.h>

#include <coreinit/title.h>
#include <coreinit/mcp.h>

#include <cstring>
#include <string>
#include <vector>

#include "utils.hpp"
#include "config.hpp"

#define OLV_TITLE_ID_JP 0x000500301001600A
#define OLV_CLIENT_ID_JP "a2efa818a34fa16b8afbc8a74eba3eda" // REPLACEME

#define OLV_TITLE_ID_US 0x000500301001610A
#define OLV_CLIENT_ID_US "a2efa818a34fa16b8afbc8a74eba3eda"

#define OLV_TITLE_ID_EU 0x000500301001620A
#define OLV_CLIENT_ID_EU "87cd32617f1985439ea608c2746e4610" // REPLACEME

namespace utils {
    MCPSystemVersion version = { .major = 0, .minor = 0, .patch = 0, .region = 'N' };

    bool isMiiverseClientID(const char *client_id) {
        return strcmp(client_id, OLV_CLIENT_ID_JP) == 0 ||
               strcmp(client_id, OLV_CLIENT_ID_US) == 0 ||
               strcmp(client_id, OLV_CLIENT_ID_EU) == 0;
    }

    bool isMiiverseTitleID(uint32_t title_id) {
        return title_id == OLV_TITLE_ID_JP || 
               title_id == OLV_TITLE_ID_US ||
               title_id == OLV_TITLE_ID_EU;
    }

    MCPSystemVersion getSystemVersion() {
        if (version.major != 0 && version.minor != 0 && version.patch != 0 && version.region != 'N') {
            return version;
        }
        int mcp = MCP_Open();
        int ret = MCP_GetSystemVersion(mcp, &version);
        if (ret < 0) {
            version = { .major = 0, .minor = 0, .patch = 0, .region = 'N' };
        }
        MCP_Close(mcp);
        return version;
    }

    char getConsoleRegion() {
        return getSystemVersion().region;
    }

    bool is555OrHigher() {
        return getSystemVersion().major == 5 && getSystemVersion().minor == 5 && getSystemVersion().patch >= 5 && (getSystemVersion().region == 'U' || getSystemVersion().region == 'E' || getSystemVersion().region == 'J');
    }

    std::string ToHexString(const void* data, size_t size)
    {
        std::string str;
        for (size_t i = 0; i < size; ++i)
            str += utils::sprintf("%02x", ((const uint8_t*) data)[i]);

        return str;
    }
}; // namespace utils