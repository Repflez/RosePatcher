#include <wups.h>
#include <wups/config_api.h>

#include <coreinit/title.h>
#include <curl/curl.h>
#include <function_patcher/function_patching.h>
#include <nn/ac.h>
#include <nn/act/client_cpp.h>
#include <nn/result.h>
#include <notifications/notifications.h>
#include <mocha/mocha.h>

#include "config.hpp"
#include "tokenthread.hpp"
#include "utils/Notification.hpp"
#include "utils/logger.h"
#include "utils/token.hpp"
#include "patches/olvFix.hpp"

WUPS_PLUGIN_NAME("rverse");
WUPS_PLUGIN_DESCRIPTION("Patcher to allow rverse access, based on Project Rosé's Patcher.");
WUPS_PLUGIN_VERSION("v0.0.1");
WUPS_PLUGIN_AUTHOR("rverseTeam & Project Rosé Team");
WUPS_PLUGIN_LICENSE("GPLv2");

WUPS_USE_STORAGE("rverse");
WUPS_USE_WUT_DEVOPTAB();

MochaUtilsStatus status;

INITIALIZE_PLUGIN() {
  // Initialize libraries
  WHBLogModuleInit();
  WHBLogUdpInit();
  WHBLogCafeInit();
  nn::act::Initialize();
  FunctionPatcher_InitLibrary();

  curl_global_init(CURL_GLOBAL_DEFAULT);
  config::InitializeConfig();

  status = Mocha_InitLibrary();
  if (status != MOCHA_RESULT_SUCCESS) {
    DEBUG("Mocha is unable ts start: 0x%8X", status);
    config::goodToGo = false;
  }

  // Check if NotificationModule library is initialized
  if (NotificationModule_InitLibrary() != NOTIFICATION_MODULE_RESULT_SUCCESS) {
    DEBUG_FUNCTION_LINE("NotificationModule_InitLibrary failed");
  }

  if (config::goodToGo) {
    if (config::connectToRverse) {
      ShowNotification("rverse patch enabled");
    } else {
      ShowNotification("rverse patch disabled");
    }
  } else {
    ShowNotification("rverse patch error. Please report this error.");
  }
}

DEINITIALIZE_PLUGIN() {
  curl_global_cleanup();

  Mocha_DeInitLibrary();
  nn::act::Finalize();
  WHBLogModuleDeinit();
  WHBLogUdpDeinit();
  WHBLogCafeDeinit();
  NotificationModule_DeInitLibrary();
  FunctionPatcher_DeInitLibrary();
}

ON_APPLICATION_START() {
  WHBLogModuleInit();
  WHBLogUdpInit();
  WHBLogCafeInit();

  if (config::goodToGo) {
    patches::olv::Initialize();

    nn::ac::Initialize();
    nn::ac::ConnectAsync();
    nn::act::Initialize();

    auto title = OSGetTitleID();

    tokenthread::CreateTokenThread();
    if (title != 0x5001010040000 && title != 0x5001010040100 && title != 0x5001010040200) {
      tokenthread::should_kill = true;
    }
  }
}

ON_APPLICATION_ENDS() {
  if (config::goodToGo) {
    auto title = OSGetTitleID();
    if (title != 0x5001010040000 && title != 0x5001010040100 && title != 0x5001010040200) {
      tokenthread::should_kill = true;
    }
  }
}

// ensure we update
DECL_FUNCTION(nn::Result, LoadConsoleAccount__Q2_2nn3actFUc13ACTLoadOptionPCcb, nn::act::SlotNo slot, nn::act::ACTLoadOption unk1, char const * unk2, bool unk3) {
  // we should load first
  nn::Result ret = real_LoadConsoleAccount__Q2_2nn3actFUc13ACTLoadOptionPCcb(slot, unk1, unk2, unk3);

  if (config::goodToGo) tokenthread::should_run_once = true;

  return ret;
}

WUPS_MUST_REPLACE(LoadConsoleAccount__Q2_2nn3actFUc13ACTLoadOptionPCcb, WUPS_LOADER_LIBRARY_NN_ACT, LoadConsoleAccount__Q2_2nn3actFUc13ACTLoadOptionPCcb);