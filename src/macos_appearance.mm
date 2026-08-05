#import <AppKit/NSAppearance.h>
#import <AppKit/NSApplication.h>

extern "C" void ForceAppLightMode(void) {
    if (@available(macOS 10.14, *)) {
        [NSApp setAppearance:[NSAppearance appearanceNamed:NSAppearanceNameAqua]];
    }
}
