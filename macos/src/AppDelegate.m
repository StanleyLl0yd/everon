#import "AppDelegate.h"

#import "HotkeyManager.h"
#import "Localization.h"
#import "PowerManager.h"

#import <Carbon/Carbon.h>
#import <CoreGraphics/CoreGraphics.h>
#import <ServiceManagement/ServiceManagement.h>
#import <UserNotifications/UserNotifications.h>

#include <math.h>

static NSString *const EveronWebsiteURL = @"https://stanleyll0yd.github.io/apps/everon/";
static NSString *const EveronPrivacyURL = @"https://stanleyll0yd.github.io/apps/everon/privacy/";

@interface AppDelegate ()
@property(nonatomic, strong) NSStatusItem *statusItem;
@property(nonatomic, strong) NSMenuItem *statusMenuItem;
@property(nonatomic, strong) PowerManager *powerManager;
@property(nonatomic, strong) HotkeyManager *hotkeyManager;
@property(nonatomic, strong) NSTimer *tickTimer;
@property(nonatomic, strong) NSTimer *keyPressTimer;
@property(nonatomic, strong) NSDate *timerEnd;
@property(nonatomic) BOOL enabled;
@end

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)notification {
    (void)notification;
    [NSApp setActivationPolicy:NSApplicationActivationPolicyAccessory];

    [NSUserDefaults.standardUserDefaults registerDefaults:@{
        @"Enabled": @NO,
        @"KeepDisplayAwake": @NO,
        @"NotifyOnToggle": @NO,
        @"KeyPress": @0,
        @"KeyPressInterval": @60,
        @"LaunchAtLogin": @NO,
        @"Language": @"system",
        @"HotkeyPreset": @0
    }];

    self.powerManager = [PowerManager new];
    self.hotkeyManager = [HotkeyManager new];
    __weak AppDelegate *weakSelf = self;
    self.hotkeyManager.handler = ^{
        [weakSelf toggleEnabled:nil];
    };

    self.statusItem = [NSStatusBar.systemStatusBar statusItemWithLength:NSVariableStatusItemLength];
    self.enabled = [NSUserDefaults.standardUserDefaults boolForKey:@"Enabled"];
    self.timerEnd = [NSUserDefaults.standardUserDefaults objectForKey:@"TimerEnd"];
    if (self.timerEnd && [self.timerEnd timeIntervalSinceNow] <= 0) {
        self.timerEnd = nil;
        self.enabled = NO;
        [self persistState];
    }

    if (![self.hotkeyManager registerPreset:[NSUserDefaults.standardUserDefaults integerForKey:@"HotkeyPreset"]]) {
        [NSUserDefaults.standardUserDefaults setInteger:0 forKey:@"HotkeyPreset"];
    }

    [self applyPowerState];
    [self rebuildMenu];
    [self updateKeyPressTimer];
    self.tickTimer = [NSTimer scheduledTimerWithTimeInterval:1.0
                                                     target:self
                                                   selector:@selector(tick:)
                                                   userInfo:nil
                                                    repeats:YES];
}

- (void)applicationWillTerminate:(NSNotification *)notification {
    (void)notification;
    [self.powerManager setEnabled:NO keepDisplayAwake:NO error:nil];
}

- (void)rebuildMenu {
    NSMenu *menu = [NSMenu new];
    self.statusMenuItem = [[NSMenuItem alloc] initWithTitle:@"" action:nil keyEquivalent:@""];
    self.statusMenuItem.enabled = NO;
    [menu addItem:self.statusMenuItem];
    [menu addItem:NSMenuItem.separatorItem];

    NSString *toggleTitle = [Localization string:self.enabled ? @"menu_disable" : @"menu_enable"];
    NSMenuItem *toggle = [[NSMenuItem alloc] initWithTitle:toggleTitle
                                                   action:@selector(toggleEnabled:)
                                            keyEquivalent:@""];
    toggle.target = self;
    [menu addItem:toggle];

    NSMenuItem *timerRoot = [[NSMenuItem alloc] initWithTitle:[Localization string:@"menu_timer"]
                                                       action:nil
                                                keyEquivalent:@""];
    NSMenu *timerMenu = [NSMenu new];
    [self addTimerItem:[Localization string:@"timer_15"] minutes:15 menu:timerMenu];
    [self addTimerItem:[Localization string:@"timer_30"] minutes:30 menu:timerMenu];
    [self addTimerItem:[Localization string:@"timer_60"] minutes:60 menu:timerMenu];
    [self addTimerItem:[Localization string:@"timer_120"] minutes:120 menu:timerMenu];
    [timerMenu addItem:NSMenuItem.separatorItem];

    NSMenuItem *custom = [[NSMenuItem alloc] initWithTitle:[Localization string:@"timer_custom"]
                                                    action:@selector(showCustomTimer:)
                                             keyEquivalent:@""];
    custom.target = self;
    [timerMenu addItem:custom];
    NSMenuItem *until = [[NSMenuItem alloc] initWithTitle:[Localization string:@"timer_until"]
                                                   action:@selector(showUntilTimer:)
                                            keyEquivalent:@""];
    until.target = self;
    [timerMenu addItem:until];
    timerRoot.submenu = timerMenu;
    [menu addItem:timerRoot];

    [menu addItem:NSMenuItem.separatorItem];
    NSMenuItem *settings = [[NSMenuItem alloc] initWithTitle:[Localization string:@"menu_settings"]
                                                      action:@selector(showSettings:)
                                               keyEquivalent:@","];
    settings.target = self;
    [menu addItem:settings];

    NSMenuItem *about = [[NSMenuItem alloc] initWithTitle:[Localization string:@"menu_about"]
                                                   action:@selector(showAbout:)
                                            keyEquivalent:@""];
    about.target = self;
    [menu addItem:about];
    [menu addItem:NSMenuItem.separatorItem];

    NSMenuItem *quit = [[NSMenuItem alloc] initWithTitle:[Localization string:@"menu_quit"]
                                                  action:@selector(terminate:)
                                           keyEquivalent:@"q"];
    quit.target = NSApp;
    [menu addItem:quit];

    self.statusItem.menu = menu;
    [self updateStatus];
}

- (void)addTimerItem:(NSString *)title minutes:(NSInteger)minutes menu:(NSMenu *)menu {
    NSMenuItem *item = [[NSMenuItem alloc] initWithTitle:title
                                                 action:@selector(startQuickTimer:)
                                          keyEquivalent:@""];
    item.target = self;
    item.tag = minutes;
    [menu addItem:item];
}

- (void)updateStatus {
    NSString *status = [Localization string:self.enabled ? @"status_enabled" : @"status_disabled"];
    if (self.enabled && self.timerEnd) {
        NSTimeInterval remaining = [self.timerEnd timeIntervalSinceNow];
        if (remaining < 0) {
            remaining = 0;
        }
        NSInteger minutes = (NSInteger)ceil(remaining / 60.0);
        status = [NSString stringWithFormat:[Localization string:@"status_remaining"], status, (long)minutes];
    }
    self.statusMenuItem.title = status;
    self.statusItem.button.toolTip = status;

    NSImage *image = [NSApp.applicationIconImage copy];
    image.size = NSMakeSize(18.0, 18.0);
    image.template = NO;
    self.statusItem.button.image = image;
}

- (void)toggleEnabled:(id)sender {
    (void)sender;
    [self setEnabled:!self.enabled clearTimer:YES notify:YES];
}

- (void)setEnabled:(BOOL)enabled clearTimer:(BOOL)clearTimer notify:(BOOL)notify {
    self.enabled = enabled;
    if (clearTimer) {
        self.timerEnd = nil;
    }
    [self persistState];

    if (![self applyPowerState]) {
        self.enabled = NO;
        self.timerEnd = nil;
        [self persistState];
    } else if (notify) {
        [self notify:[Localization string:self.enabled ? @"notification_enabled" : @"notification_disabled"]];
    }

    [self updateKeyPressTimer];
    [self rebuildMenu];
}

- (BOOL)applyPowerState {
    NSError *error = nil;
    BOOL keepDisplay = [NSUserDefaults.standardUserDefaults boolForKey:@"KeepDisplayAwake"];
    if (![self.powerManager setEnabled:self.enabled keepDisplayAwake:keepDisplay error:&error]) {
        [self showError:[Localization string:@"power_error"]];
        return NO;
    }
    return YES;
}

- (void)persistState {
    [NSUserDefaults.standardUserDefaults setBool:self.enabled forKey:@"Enabled"];
    if (self.timerEnd) {
        [NSUserDefaults.standardUserDefaults setObject:self.timerEnd forKey:@"TimerEnd"];
    } else {
        [NSUserDefaults.standardUserDefaults removeObjectForKey:@"TimerEnd"];
    }
}

- (void)startQuickTimer:(NSMenuItem *)sender {
    [self startTimerForMinutes:sender.tag];
}

- (void)startTimerForMinutes:(NSInteger)minutes {
    self.timerEnd = [NSDate dateWithTimeIntervalSinceNow:minutes * 60.0];
    self.enabled = YES;
    [self persistState];
    if (![self applyPowerState]) {
        self.enabled = NO;
        self.timerEnd = nil;
        [self persistState];
    } else {
        [self notify:[Localization string:@"notification_enabled"]];
    }
    [self updateKeyPressTimer];
    [self rebuildMenu];
}

- (void)showCustomTimer:(id)sender {
    (void)sender;
    NSAlert *alert = [NSAlert new];
    alert.messageText = [Localization string:@"custom_title"];
    alert.informativeText = [Localization string:@"custom_prompt"];
    [alert addButtonWithTitle:[Localization string:@"common_ok"]];
    [alert addButtonWithTitle:[Localization string:@"common_cancel"]];

    NSTextField *field = [[NSTextField alloc] initWithFrame:NSMakeRect(0, 0, 160, 24)];
    field.stringValue = @"60";
    alert.accessoryView = field;
    [NSApp activateIgnoringOtherApps:YES];

    if ([alert runModal] != NSAlertFirstButtonReturn) {
        return;
    }
    NSInteger minutes = field.integerValue;
    if (minutes < 5 || minutes > 1440) {
        [self showError:[Localization string:@"invalid_duration"]];
        return;
    }
    [self startTimerForMinutes:minutes];
}

- (void)showUntilTimer:(id)sender {
    (void)sender;
    NSAlert *alert = [NSAlert new];
    alert.messageText = [Localization string:@"until_title"];
    [alert addButtonWithTitle:[Localization string:@"common_ok"]];
    [alert addButtonWithTitle:[Localization string:@"common_cancel"]];

    NSDatePicker *picker = [[NSDatePicker alloc] initWithFrame:NSMakeRect(0, 0, 180, 28)];
    picker.datePickerStyle = NSDatePickerStyleTextFieldAndStepper;
    picker.datePickerElements = NSHourMinuteDatePickerElementFlag;
    picker.dateValue = [NSDate dateWithTimeIntervalSinceNow:3600];
    alert.accessoryView = picker;
    [NSApp activateIgnoringOtherApps:YES];

    if ([alert runModal] != NSAlertFirstButtonReturn) {
        return;
    }

    NSCalendar *calendar = NSCalendar.currentCalendar;
    NSDate *now = NSDate.date;
    NSDateComponents *time = [calendar components:(NSCalendarUnitHour | NSCalendarUnitMinute)
                                          fromDate:picker.dateValue];
    NSDateComponents *today = [calendar components:(NSCalendarUnitYear | NSCalendarUnitMonth | NSCalendarUnitDay)
                                           fromDate:now];
    today.hour = time.hour;
    today.minute = time.minute;
    today.second = 0;
    NSDate *end = [calendar dateFromComponents:today];
    if ([end compare:now] != NSOrderedDescending) {
        end = [calendar dateByAddingUnit:NSCalendarUnitDay value:1 toDate:end options:0];
    }

    self.timerEnd = end;
    self.enabled = YES;
    [self persistState];
    if (![self applyPowerState]) {
        self.enabled = NO;
        self.timerEnd = nil;
        [self persistState];
    } else {
        [self notify:[Localization string:@"notification_enabled"]];
    }
    [self updateKeyPressTimer];
    [self rebuildMenu];
}

- (void)tick:(NSTimer *)timer {
    (void)timer;
    if (self.enabled && self.timerEnd && [self.timerEnd timeIntervalSinceNow] <= 0) {
        [self setEnabled:NO clearTimer:YES notify:NO];
        [self notify:[Localization string:@"notification_expired"]];
        return;
    }
    [self updateStatus];
}

- (void)updateKeyPressTimer {
    [self.keyPressTimer invalidate];
    self.keyPressTimer = nil;
    NSInteger key = [NSUserDefaults.standardUserDefaults integerForKey:@"KeyPress"];
    if (!self.enabled || key == 0) {
        return;
    }

    NSTimeInterval interval = [NSUserDefaults.standardUserDefaults doubleForKey:@"KeyPressInterval"];
    if (interval < 1) {
        interval = 1;
    } else if (interval > 86400) {
        interval = 86400;
    }
    self.keyPressTimer = [NSTimer scheduledTimerWithTimeInterval:interval
                                                         target:self
                                                       selector:@selector(sendSyntheticKey:)
                                                       userInfo:nil
                                                        repeats:YES];
}

- (void)sendSyntheticKey:(NSTimer *)timer {
    (void)timer;
    NSInteger setting = [NSUserDefaults.standardUserDefaults integerForKey:@"KeyPress"];
    CGKeyCode keyCode = 0;
    if (setting == 1) {
        keyCode = kVK_F15;
    } else if (setting == 2) {
        keyCode = kVK_F16;
    } else if (setting == 3) {
        keyCode = kVK_F17;
    } else {
        return;
    }

    CGEventRef down = CGEventCreateKeyboardEvent(NULL, keyCode, true);
    CGEventRef up = CGEventCreateKeyboardEvent(NULL, keyCode, false);
    if (down && up) {
        CGEventPost(kCGHIDEventTap, down);
        CGEventPost(kCGHIDEventTap, up);
    }
    if (down) {
        CFRelease(down);
    }
    if (up) {
        CFRelease(up);
    }
}

- (void)showSettings:(id)sender {
    (void)sender;
    NSUserDefaults *defaults = NSUserDefaults.standardUserDefaults;
    NSAlert *alert = [NSAlert new];
    alert.messageText = [Localization string:@"settings_title"];
    [alert addButtonWithTitle:[Localization string:@"common_ok"]];
    [alert addButtonWithTitle:[Localization string:@"common_cancel"]];

    NSButton *display = [NSButton checkboxWithTitle:[Localization string:@"settings_keep_display"] target:nil action:nil];
    display.state = [defaults boolForKey:@"KeepDisplayAwake"] ? NSControlStateValueOn : NSControlStateValueOff;
    NSButton *notifications = [NSButton checkboxWithTitle:[Localization string:@"settings_notifications"] target:nil action:nil];
    notifications.state = [defaults boolForKey:@"NotifyOnToggle"] ? NSControlStateValueOn : NSControlStateValueOff;
    NSButton *login = [NSButton checkboxWithTitle:[Localization string:@"settings_launch_login"] target:nil action:nil];
    login.state = [defaults boolForKey:@"LaunchAtLogin"] ? NSControlStateValueOn : NSControlStateValueOff;

    NSPopUpButton *keyPress = [[NSPopUpButton alloc] initWithFrame:NSZeroRect pullsDown:NO];
    [keyPress addItemsWithTitles:@[[Localization string:@"common_off"], @"F15", @"F16", @"F17"]];
    [keyPress selectItemAtIndex:[defaults integerForKey:@"KeyPress"]];

    NSTextField *interval = [NSTextField textFieldWithString:[NSString stringWithFormat:@"%ld", (long)[defaults integerForKey:@"KeyPressInterval"]]];

    NSPopUpButton *language = [[NSPopUpButton alloc] initWithFrame:NSZeroRect pullsDown:NO];
    NSArray<NSString *> *languageCodes = @[@"system", @"en", @"ru", @"fr", @"de", @"it", @"es"];
    [language addItemsWithTitles:@[[Localization string:@"language_system"], @"English", @"Русский", @"Français", @"Deutsch", @"Italiano", @"Español"]];
    NSInteger languageIndex = [languageCodes indexOfObject:[Localization languageCode]];
    [language selectItemAtIndex:languageIndex == NSNotFound ? 0 : languageIndex];

    NSPopUpButton *hotkey = [[NSPopUpButton alloc] initWithFrame:NSZeroRect pullsDown:NO];
    [hotkey addItemsWithTitles:@[[Localization string:@"common_off"], @"⌃⌥E", @"⌃⌥⌘E"]];
    [hotkey selectItemAtIndex:[defaults integerForKey:@"HotkeyPreset"]];

    NSGridView *grid = [NSGridView gridViewWithViews:@[
        @[[NSTextField labelWithString:@""], display],
        @[[NSTextField labelWithString:@""], notifications],
        @[[NSTextField labelWithString:@""], login],
        @[[NSTextField labelWithString:[Localization string:@"settings_key_press"]], keyPress],
        @[[NSTextField labelWithString:[Localization string:@"settings_interval"]], interval],
        @[[NSTextField labelWithString:[Localization string:@"settings_hotkey"]], hotkey],
        @[[NSTextField labelWithString:[Localization string:@"settings_language"]], language]
    ]];
    grid.rowSpacing = 8;
    grid.columnSpacing = 12;
    grid.frame = NSMakeRect(0, 0, 390, 210);
    alert.accessoryView = grid;
    [NSApp activateIgnoringOtherApps:YES];

    if ([alert runModal] != NSAlertFirstButtonReturn) {
        return;
    }

    NSInteger intervalValue = interval.integerValue;
    if (intervalValue < 1 || intervalValue > 86400) {
        [self showError:[Localization string:@"invalid_interval"]];
        return;
    }

    BOOL oldLogin = [defaults boolForKey:@"LaunchAtLogin"];
    BOOL newLogin = login.state == NSControlStateValueOn;
    if (newLogin != oldLogin && ![self setLaunchAtLogin:newLogin]) {
        return;
    }

    NSInteger hotkeyPreset = hotkey.indexOfSelectedItem;
    if (![self.hotkeyManager registerPreset:hotkeyPreset]) {
        [self showError:[Localization string:@"hotkey_error"]];
        return;
    }

    [defaults setBool:display.state == NSControlStateValueOn forKey:@"KeepDisplayAwake"];
    [defaults setBool:notifications.state == NSControlStateValueOn forKey:@"NotifyOnToggle"];
    [defaults setBool:newLogin forKey:@"LaunchAtLogin"];
    [defaults setInteger:keyPress.indexOfSelectedItem forKey:@"KeyPress"];
    [defaults setInteger:intervalValue forKey:@"KeyPressInterval"];
    [defaults setInteger:hotkeyPreset forKey:@"HotkeyPreset"];
    [Localization setLanguageCode:languageCodes[language.indexOfSelectedItem]];

    if (notifications.state == NSControlStateValueOn) {
        [UNUserNotificationCenter.currentNotificationCenter requestAuthorizationWithOptions:(UNAuthorizationOptionAlert | UNAuthorizationOptionSound)
                                                                           completionHandler:^(BOOL granted, NSError *error) {
            (void)granted;
            (void)error;
        }];
    }

    [self applyPowerState];
    [self updateKeyPressTimer];
    [self rebuildMenu];
}

- (BOOL)setLaunchAtLogin:(BOOL)enabled {
    NSError *error = nil;
    SMAppService *service = SMAppService.mainAppService;
    BOOL success = enabled ? [service registerAndReturnError:&error] : [service unregisterAndReturnError:&error];
    if (!success) {
        NSString *message = error.localizedDescription;
        [self showError:message ? message : [Localization string:@"launch_error"]];
    }
    return success;
}

- (void)showAbout:(id)sender {
    (void)sender;
    NSString *version = [NSBundle.mainBundle objectForInfoDictionaryKey:@"CFBundleShortVersionString"];
    if (!version) {
        version = @"";
    }
    NSAlert *alert = [NSAlert new];
    alert.messageText = [NSString stringWithFormat:@"Everon %@", version];
    alert.informativeText = [NSString stringWithFormat:@"%@\n\nStanley Lloyd\nPolyForm Noncommercial 1.0.0",
                             [Localization string:@"about_tagline"]];
    [alert addButtonWithTitle:[Localization string:@"common_ok"]];
    [alert addButtonWithTitle:[Localization string:@"about_website"]];
    [alert addButtonWithTitle:[Localization string:@"about_privacy"]];
    [NSApp activateIgnoringOtherApps:YES];

    NSModalResponse response = [alert runModal];
    if (response == NSAlertSecondButtonReturn) {
        [NSWorkspace.sharedWorkspace openURL:[NSURL URLWithString:EveronWebsiteURL]];
    } else if (response == NSAlertThirdButtonReturn) {
        [NSWorkspace.sharedWorkspace openURL:[NSURL URLWithString:EveronPrivacyURL]];
    }
}

- (void)notify:(NSString *)message {
    if (![NSUserDefaults.standardUserDefaults boolForKey:@"NotifyOnToggle"]) {
        return;
    }
    UNMutableNotificationContent *content = [UNMutableNotificationContent new];
    content.title = @"Everon";
    content.body = message;
    UNNotificationRequest *request = [UNNotificationRequest requestWithIdentifier:NSUUID.UUID.UUIDString
                                                                          content:content
                                                                          trigger:nil];
    [UNUserNotificationCenter.currentNotificationCenter addNotificationRequest:request withCompletionHandler:nil];
}

- (void)showError:(NSString *)message {
    NSAlert *alert = [NSAlert new];
    alert.alertStyle = NSAlertStyleWarning;
    alert.messageText = @"Everon";
    alert.informativeText = message;
    [alert addButtonWithTitle:[Localization string:@"common_ok"]];
    [NSApp activateIgnoringOtherApps:YES];
    [alert runModal];
}

@end
