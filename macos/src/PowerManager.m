#import "PowerManager.h"

#import <IOKit/pwr_mgt/IOPMLib.h>

@implementation PowerManager {
    IOPMAssertionID _systemAssertion;
    IOPMAssertionID _displayAssertion;
}

- (instancetype)init {
    self = [super init];
    if (self) {
        _systemAssertion = kIOPMNullAssertionID;
        _displayAssertion = kIOPMNullAssertionID;
    }
    return self;
}

- (void)releaseAssertions {
    if (_displayAssertion != kIOPMNullAssertionID) {
        IOPMAssertionRelease(_displayAssertion);
        _displayAssertion = kIOPMNullAssertionID;
    }
    if (_systemAssertion != kIOPMNullAssertionID) {
        IOPMAssertionRelease(_systemAssertion);
        _systemAssertion = kIOPMNullAssertionID;
    }
}

- (BOOL)setEnabled:(BOOL)enabled keepDisplayAwake:(BOOL)keepDisplayAwake error:(NSError **)error {
    [self releaseAssertions];
    if (!enabled) {
        return YES;
    }

    IOReturn result = IOPMAssertionCreateWithName(kIOPMAssertionTypePreventUserIdleSystemSleep,
                                                   kIOPMAssertionLevelOn,
                                                   CFSTR("Everon is keeping the Mac awake"),
                                                   &_systemAssertion);
    if (result != kIOReturnSuccess) {
        if (error) {
            *error = [NSError errorWithDomain:@"io.github.stanleyll0yd.everon.power"
                                         code:result
                                     userInfo:nil];
        }
        return NO;
    }

    if (keepDisplayAwake) {
        result = IOPMAssertionCreateWithName(kIOPMAssertionTypePreventUserIdleDisplaySleep,
                                             kIOPMAssertionLevelOn,
                                             CFSTR("Everon is keeping the display awake"),
                                             &_displayAssertion);
        if (result != kIOReturnSuccess) {
            [self releaseAssertions];
            if (error) {
                *error = [NSError errorWithDomain:@"io.github.stanleyll0yd.everon.power"
                                             code:result
                                         userInfo:nil];
            }
            return NO;
        }
    }

    return YES;
}

- (void)dealloc {
    [self releaseAssertions];
}

@end
