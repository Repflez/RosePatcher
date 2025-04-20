#include <wups.h>
#include <coreinit/title.h>
#include <function_patcher/function_patching.h>
#include <notifications/notifications.h>

#include "config.hpp"
#include "patches/tviiIcon.hpp"
#include "utils/Notification.hpp"
#include "utils/logger.h"

WUPS_PLUGIN_NAME("Rosé Patcher");
WUPS_PLUGIN_DESCRIPTION("Patcher for Project Rosé's Nintendo TVii revival service.");
WUPS_PLUGIN_VERSION("v1.1.0");
WUPS_PLUGIN_AUTHOR("Project Rosé Team");
WUPS_PLUGIN_LICENSE("GPLv2");

WUPS_USE_STORAGE("rosepatcher");
WUPS_USE_WUT_DEVOPTAB();

INITIALIZE_PLUGIN() {
  // Initialize libraries
  WHBLogModuleInit();
  WHBLogUdpInit();
  WHBLogCafeInit();
  FunctionPatcher_InitLibrary();

  config::InitializeConfig();

  // Check if NotificationModule library is initialized
  if (NotificationModule_InitLibrary() != NOTIFICATION_MODULE_RESULT_SUCCESS) {
    DEBUG_FUNCTION_LINE("NotificationModule_InitLibrary failed");
  }

  if (config::connectToRose) {
    ShowNotification("Rosé patch enabled");
  } else {
    ShowNotification("Rosé patch disabled");
  }
}

DEINITIALIZE_PLUGIN() {
  patches::icon::perform_hbm_patches(false);

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

  auto title = OSGetTitleID();
  if (config::tviiIconWUM) {
    if (title == 0x5001010040000 || title == 0x5001010040100 || title == 0x5001010040200) {
      patches::icon::perform_men_patches(true);
    }
  }
}

ON_APPLICATION_ENDS() {
  auto title = OSGetTitleID();
  if (title == 0x5001010040000 || title == 0x5001010040100 || title == 0x5001010040200) {
    patches::icon::perform_men_patches(false);
  }
}