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

#define CONNECT_TO_ROSE_CONFIG_ID "connect_to_rose"
#define CONNECT_TO_ROSE_DEFUALT_VALUE true

#define TVII_ICON_HBM_PATCH_COFNIG_ID "tvii_icon_hbm_patch"
#define TVII_ICON_HBM_PATCH_DEFAULT_VALUE true

#define TVII_ICON_WUM_PATCH_COFNIG_ID "tvii_icon_wum_patch"
#define TVII_ICON_WUM_PATCH_DEFAULT_VALUE true

#define FORCE_JPN_CONSOLE_CONFIG_ID "force_jpn_console"
#define FORCE_JPN_CONSOLE_DEFAULT_VALUE false



namespace config {
    // This isn't configuration, but we need to create the replacement token ASAP
    // since I don't think we can generate it while in vino -ItzSwirlz
    extern std::string replacementToken;

    extern bool connectToRose;
    extern bool tviiIconHBM;
    extern bool tviiIconWUM;
    extern bool needRelaunch;
    extern bool forceJPNconsole;
    extern bool certificateAdded;
    extern bool gtsAdded;

    void connectToRoseChanged(ConfigItemBoolean *item, bool newValue);
    void tviiIconHBMChanged(ConfigItemBoolean *item, bool newValue);
    void tviiIconWUMChanged(ConfigItemBoolean *item, bool newValue);

    WUPSConfigAPICallbackStatus ConfigMenuOpenedCallback(WUPSConfigCategoryHandle rootHandle);
    
    void ConfigMenuClosedCallback();
    void InitializeConfig();
} // namespace config
