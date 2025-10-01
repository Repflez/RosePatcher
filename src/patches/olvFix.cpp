#include <wups.h>

#include <coreinit/dynload.h>
#include <coreinit/filesystem.h>
#include <coreinit/memory.h>

#include <cstring>
#include <string>
#include <vector>
#include <optional>

#include "olvFix.hpp"
#include "config.hpp"
#include "utils/patch.hpp"
#include "utils/logger.h"

#include "chain_pem.h" // generated at buildtime

static std::optional<FSFileHandle> rootCAPemHandle{};

const char wave_original[] = {
        0x68, 0x74, 0x74, 0x70, 0x73, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x2E, 0x6E, 0x69, 0x6E, 0x74, 0x65, 0x6E, 0x64,
        0x6F, 0x2E, 0x6E, 0x65, 0x74
};
const char wave_new[] = {
		0x68, 0x74, 0x74, 0x70, 0x73, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x2E, 0x72, 0x76, 0x65, 0x72, 0x73, 0x65, 0x2E,
		0x63, 0x6C, 0x75, 0x62, 0x00
};

namespace patches::olv {
    const char original_discovery_url[] = "discovery.olv.nintendo.net/v1/endpoint";
    const char new_discovery_url[]      = SERVER_URL;

    _Static_assert(sizeof(original_discovery_url) > sizeof(new_discovery_url),
                    "new_discovery_url too long! Must be less than 38chars.");

    void osdynload_notify_callback(OSDynLoad_Module module, void *ctx,
                                   OSDynLoad_NotifyReason reason, OSDynLoad_NotifyData *rpl) {
        if (reason == OS_DYNLOAD_NOTIFY_LOADED) {
            if (!config::goodToGo) return;
            if (!rpl->name) return;
            if (!std::string_view(rpl->name).ends_with("nn_olv.rpl") || !config::connectToRverse)
                return;

            utils::patch::replace_mem(rpl->dataAddr, rpl->dataSize, original_discovery_url,
                        sizeof(original_discovery_url), new_discovery_url, sizeof(new_discovery_url));
        }
    }

    bool CheckForOlvLibraries() {
        OSDynLoad_Module handle = 0;
        OSDynLoad_Error err;

        err = OSDynLoad_IsModuleLoaded("nn_olv", &handle);
        if (err == OS_DYNLOAD_OK && handle != 0) return true;

        err = OSDynLoad_IsModuleLoaded("nn_olv2", &handle);
        if (err == OS_DYNLOAD_OK && handle != 0) return true;

        return false;
    }

    bool Initialize() {
        if (config::connectToRverse && config::goodToGo) {
            OSDynLoad_AddNotifyCallback(&patches::olv::osdynload_notify_callback, nullptr);

            bool loadedLibs = CheckForOlvLibraries();
            if (!loadedLibs) {
                DEBUG_FUNCTION_LINE("no olv libraries loaded, bailing out");
                return false;
            }

            // Hoping jon doesn't fuck me over this
            uint32_t baseAddr, boundSize;
            if (OSGetMemBound(OS_MEM2, &baseAddr, &boundSize)) {
                DEBUG_FUNCTION_LINE("OSGetMemBound failed to run. wtf");
                return false;
            }

            return utils::patch::replace_mem(baseAddr, boundSize, original_discovery_url, sizeof(original_discovery_url),
                                    new_discovery_url, sizeof(new_discovery_url));
        }

        return false;
    }
} // namespace olv

DECL_FUNCTION(int, FSOpenFile_OLV, FSClient *client, FSCmdBlock *block, char *path, const char *mode, uint32_t *handle,
              int error) {
    const char *initialOmaPath = "vol/content/initial.oma";

    if (config::connectToRverse && config::goodToGo) {
        if (strcmp(initialOmaPath, path) == 0) {
            // This is a hack
            auto ok = patches::olv::Initialize();

            if (ok)
                utils::patch::replace_mem(0x10000000, 0x10000000, wave_original, sizeof(wave_original), wave_new, sizeof(wave_new));
        } else if (strcmp("vol/content/browser/rootca.pem", path) == 0) {
            // The CA chain is added here :)
            int ret = real_FSOpenFile_OLV(client, block, path, mode, handle, error);

            rootCAPemHandle = *handle;

            DEBUG_FUNCTION_LINE("replacing Miiverse CA with bundled chain");

            return ret;
        }
    } else {
        DEBUG_FUNCTION_LINE("patches disabled, skipping process");
    }

    return real_FSOpenFile_OLV(client, block, path, mode, handle, error);
}

DECL_FUNCTION(FSStatus, FSReadFile_OLV, FSClient *client, FSCmdBlock *block, uint8_t *buffer, uint32_t size, uint32_t count,
              FSFileHandle handle, uint32_t unk1, uint32_t flags) {
    if (config::connectToRverse && config::goodToGo) {
        if (size != 1) {
            DEBUG_FUNCTION_LINE("Miiverse CA replacement failed");
        }

        if (rootCAPemHandle && *rootCAPemHandle == handle) {
            strlcpy((char *) buffer, (const char *) chain_pem, size * count);

            return (FSStatus) count;
        }
    }

    return real_FSReadFile_OLV(client, block, buffer, size, count, handle, unk1, flags);
}

DECL_FUNCTION(FSStatus, FSCloseFile_OLV, FSClient *client, FSCmdBlock *block, FSFileHandle handle, FSErrorFlag errorMask) {
    if (config::connectToRverse && config::goodToGo) {
        if (handle == rootCAPemHandle) {
            rootCAPemHandle.reset();
        }
    }

    return real_FSCloseFile_OLV(client, block, handle, errorMask);
}

WUPS_MUST_REPLACE_FOR_PROCESS(FSOpenFile_OLV, WUPS_LOADER_LIBRARY_COREINIT, FSOpenFile, WUPS_FP_TARGET_PROCESS_MIIVERSE);
WUPS_MUST_REPLACE_FOR_PROCESS(FSReadFile_OLV, WUPS_LOADER_LIBRARY_COREINIT, FSReadFile, WUPS_FP_TARGET_PROCESS_MIIVERSE);
WUPS_MUST_REPLACE_FOR_PROCESS(FSCloseFile_OLV, WUPS_LOADER_LIBRARY_COREINIT, FSCloseFile, WUPS_FP_TARGET_PROCESS_MIIVERSE);