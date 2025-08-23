#include <algorithm>
#include <format>
#include <wups.h>
#include <wups/config.h>
#include <wups/config/WUPSConfigCategory.h>
#include <wups/config/WUPSConfigItem.h>
#include <wups/config/WUPSConfigItemBoolean.h>
#include <wups/config/WUPSConfigItemIntegerRange.h>
#include <wups/config/WUPSConfigItemMultipleValues.h>
#include <wups/config/WUPSConfigItemStub.h>
#include <coreinit/launch.h>
#include <sysapp/launch.h>
#include <sysapp/title.h>

#include "config.hpp"
#include "utils/utils.hpp"
#include "utils/logger.h"
#include "utils/token.hpp"

namespace config {
  bool connectToRverse = CONNECT_TO_RVERSE_DEFUALT_VALUE;
  bool needRelaunch = false;
  bool certificateAdded = false;
  bool gtsAdded = false;
  bool enableRemindPoll = true;

  // Connect to rverse setting event
  void connectToRverseChanged(ConfigItemBoolean *item, bool newValue) {
    if (newValue != connectToRverse) {
      WUPSStorageAPI::Store(CONNECT_TO_RVERSE_CONFIG_ID, newValue);
    }

    connectToRverse = newValue;
  }

  // Open event for the Aroma config menu
  WUPSConfigAPICallbackStatus ConfigMenuOpenedCallback(WUPSConfigCategoryHandle rootHandle) {
    WUPSConfigCategory root = WUPSConfigCategory(rootHandle);

    try {
      // Add setting items
      root.add(WUPSConfigItemBoolean::Create(CONNECT_TO_RVERSE_CONFIG_ID, "Connect to rverse", CONNECT_TO_RVERSE_DEFUALT_VALUE, connectToRverse, connectToRverseChanged));
    } catch (std::exception &e) {
      DEBUG_FUNCTION_LINE("Creating config menu failed: %s", e.what());
      return WUPSCONFIG_API_CALLBACK_RESULT_ERROR;
    }

    return WUPSCONFIG_API_CALLBACK_RESULT_SUCCESS;
  }

  // Close event for the Aroma config menu
  void ConfigMenuClosedCallback() {
    WUPSStorageAPI::SaveStorage();
    if (config::needRelaunch) {
      OSForceFullRelaunch();
      SYSLaunchMenu();
      config::needRelaunch = false;
    }
  }

  // Config stuff in plugin initialization
  void InitializeConfig() {
      // Add the config
    WUPSConfigAPIOptionsV1 configOptions = {.name = "rverse"};
    if (WUPSConfigAPI_Init(configOptions, ConfigMenuOpenedCallback, ConfigMenuClosedCallback) != WUPSCONFIG_API_RESULT_SUCCESS) {
      DEBUG_FUNCTION_LINE("Failed to init config api");
    }

    // Add get saved values
    WUPSStorageError storageRes;
    if ((storageRes = WUPSStorageAPI::GetOrStoreDefault(CONNECT_TO_RVERSE_CONFIG_ID, connectToRverse, CONNECT_TO_RVERSE_DEFUALT_VALUE)) != WUPS_STORAGE_ERROR_SUCCESS) {
      DEBUG_FUNCTION_LINE("GetOrStoreDefault failed: %s (%d)", WUPSStorageAPI_GetStatusStr(storageRes), storageRes);
    }

    token::initToken();
  }

} // namespace config
