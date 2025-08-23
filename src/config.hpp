#pragma once
#include <format>

#include <wups.h>
#include <wups/config.h>
#include <wups/config/WUPSConfigCategory.h>
#include <wups/config/WUPSConfigItem.h>
#include <wups/config/WUPSConfigItemBoolean.h>
#include <wups/config/WUPSConfigItemIntegerRange.h>
#include <wups/config/WUPSConfigItemMultipleValues.h>
#include <wups/config/WUPSConfigItemStub.h>

#define CONNECT_TO_RVERSE_CONFIG_ID "connect_to_rverse"
#define CONNECT_TO_RVERSE_DEFUALT_VALUE true

namespace config {

    extern bool connectToRverse;
    extern bool needRelaunch;
    extern bool certificateAdded;
    extern bool gtsAdded;
    extern bool enableRemindPoll;

    void connectToRverseChanged(ConfigItemBoolean *item, bool newValue);

    WUPSConfigAPICallbackStatus ConfigMenuOpenedCallback(WUPSConfigCategoryHandle rootHandle);
    
    void ConfigMenuClosedCallback();
    void InitializeConfig();
} // namespace config
