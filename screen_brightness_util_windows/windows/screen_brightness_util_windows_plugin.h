#ifndef FLUTTER_PLUGIN_SCREEN_BRIGHTNESS_UTIL_WINDOWS_PLUGIN_H_
#define FLUTTER_PLUGIN_SCREEN_BRIGHTNESS_UTIL_WINDOWS_PLUGIN_H_

#include <flutter/method_channel.h>
#include <flutter/plugin_registrar_windows.h>
#include <wbemidl.h>                            // provides WMI (Windows Management Instrumentation) COM interfaces. WMI is a Microsoft technology for managing and accessing system information in a Windows environment.
#include <comdef.h>                             // provides COM utility functions, including helper classes and macros for COM programming.
#include <memory>
#pragma comment(lib, "wbemuuid.lib")            // pragma directive instructs the linker to link the wbemuuid.lib library. The library contains the UUID (Universally Unique Identifier) definitions required for using WMI COM interfaces.
namespace screen_brightness_util_windows {

class ScreenBrightnessUtilWindowsPlugin : public flutter::Plugin {
 public:
  static void RegisterWithRegistrar(flutter::PluginRegistrarWindows *registrar);

  ScreenBrightnessUtilWindowsPlugin(flutter::PluginRegistrarWindows* registrar);

  virtual ~ScreenBrightnessUtilWindowsPlugin();

  // Disallow copy and assign.
  ScreenBrightnessUtilWindowsPlugin(const ScreenBrightnessUtilWindowsPlugin&) = delete;
  ScreenBrightnessUtilWindowsPlugin& operator=(const ScreenBrightnessUtilWindowsPlugin&) = delete;

 private:
  // Called when a method is called on this plugin's channel from Dart.
  void HandleMethodCall(
      const flutter::MethodCall<flutter::EncodableValue> &method_call,
      std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);
  
  float getBrightness();
  bool setBrightness(const double brightness);
  int setNotebookDisplayBrightness_WMI(uint8_t brightness);
  VARIANT getNotebookDisplayProperty_WMI(BSTR* PropertyName);
  flutter::PluginRegistrarWindows* registrar;
};

}  // namespace screen_brightness_util_windows

#endif  // FLUTTER_PLUGIN_SCREEN_BRIGHTNESS_UTIL_WINDOWS_PLUGIN_H_
