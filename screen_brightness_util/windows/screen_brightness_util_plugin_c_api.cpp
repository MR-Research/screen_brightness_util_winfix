#include "include/screen_brightness_util/screen_brightness_util_plugin_c_api.h"

#include <flutter/plugin_registrar_windows.h>

#include "screen_brightness_util_plugin.h"

void ScreenBrightnessUtilPluginCApiRegisterWithRegistrar(
    FlutterDesktopPluginRegistrarRef registrar) {
  screen_brightness_util::ScreenBrightnessUtilPlugin::RegisterWithRegistrar(
      flutter::PluginRegistrarManager::GetInstance()
          ->GetRegistrar<flutter::PluginRegistrarWindows>(registrar));
}
