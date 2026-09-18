#import "HotkeyManager.h"

#import <Carbon/Carbon.h>

@interface HotkeyManager ()
- (void)invoke;
@end

static OSStatus EveronHotKeyHandler(EventHandlerCallRef nextHandler, EventRef event, void *userData) {
    (void)nextHandler;
    (void)event;
    HotkeyManager *manager = (__bridge HotkeyManager *)userData;
    [manager invoke];
    return noErr;
}

@implementation HotkeyManager {
    EventHotKeyRef _hotKey;
    EventHandlerRef _eventHandler;
}

- (instancetype)init {
    self = [super init];
    if (self) {
        EventTypeSpec type = {kEventClassKeyboard, kEventHotKeyPressed};
        InstallApplicationEventHandler(&EveronHotKeyHandler,
                                       1,
                                       &type,
                                       (__bridge void *)self,
                                       &_eventHandler);
    }
    return self;
}

- (BOOL)registerPreset:(NSInteger)preset {
    if (_hotKey) {
        UnregisterEventHotKey(_hotKey);
        _hotKey = NULL;
    }
    if (preset == 0) {
        return YES;
    }

    UInt32 modifiers = controlKey | optionKey;
    if (preset == 2) {
        modifiers |= cmdKey;
    }
    EventHotKeyID hotKeyID = {0x4556524E, 1};
    return RegisterEventHotKey(kVK_ANSI_E,
                               modifiers,
                               hotKeyID,
                               GetApplicationEventTarget(),
                               0,
                               &_hotKey) == noErr;
}

- (void)invoke {
    if (self.handler) {
        self.handler();
    }
}

- (void)dealloc {
    if (_hotKey) {
        UnregisterEventHotKey(_hotKey);
    }
    if (_eventHandler) {
        RemoveEventHandler(_eventHandler);
    }
}

@end
