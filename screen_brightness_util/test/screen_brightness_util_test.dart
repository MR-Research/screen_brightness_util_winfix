import 'package:flutter_test/flutter_test.dart';
import 'package:screen_brightness_util/screen_brightness_util.dart';
import 'package:screen_brightness_util/screen_brightness_util_platform_interface.dart';
import 'package:screen_brightness_util/screen_brightness_util_method_channel.dart';
import 'package:plugin_platform_interface/plugin_platform_interface.dart';

class MockScreenBrightnessUtilPlatform
    with MockPlatformInterfaceMixin
    implements ScreenBrightnessUtilPlatform {

  @override
  Future<String?> getPlatformVersion() => Future.value('42');
}

void main() {
  final ScreenBrightnessUtilPlatform initialPlatform = ScreenBrightnessUtilPlatform.instance;

  test('$MethodChannelScreenBrightnessUtil is the default instance', () {
    expect(initialPlatform, isInstanceOf<MethodChannelScreenBrightnessUtil>());
  });

  test('getPlatformVersion', () async {
    ScreenBrightnessUtil screenBrightnessUtilPlugin = ScreenBrightnessUtil();
    MockScreenBrightnessUtilPlatform fakePlatform = MockScreenBrightnessUtilPlatform();
    ScreenBrightnessUtilPlatform.instance = fakePlatform;

    expect(await screenBrightnessUtilPlugin.getPlatformVersion(), '42');
  });
}
