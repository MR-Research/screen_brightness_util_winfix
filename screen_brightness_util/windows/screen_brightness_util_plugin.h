#ifndef FLUTTER_PLUGIN_SCREEN_BRIGHTNESS_UTIL_PLUGIN_H_
#define FLUTTER_PLUGIN_SCREEN_BRIGHTNESS_UTIL_PLUGIN_H_

#include <flutter/method_channel.h>
#include <flutter/plugin_registrar_windows.h>

#include <memory>

namespace screen_brightness_util {

class ScreenBrightnessUtilPlugin : public flutter::Plugin {
 public:
  static void RegisterWithRegistrar(flutter::PluginRegistrarWindows *registrar);

  ScreenBrightnessUtilPlugin();

  virtual ~ScreenBrightnessUtilPlugin();

  // Disallow copy and assign.
  ScreenBrightnessUtilPlugin(const ScreenBrightnessUtilPlugin&) = delete;
  ScreenBrightnessUtilPlugin& operator=(const ScreenBrightnessUtilPlugin&) = delete;

  // Called when a method is called on this plugin's channel from Dart.
  void HandleMethodCall(
      const flutter::MethodCall<flutter::EncodableValue> &method_call,
      std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);
};

}  // namespace screen_brightness_util

#endif  // FLUTTER_PLUGIN_SCREEN_BRIGHTNESS_UTIL_PLUGIN_H_
