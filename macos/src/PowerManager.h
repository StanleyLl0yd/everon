#pragma once

#import <Foundation/Foundation.h>

@interface PowerManager : NSObject
- (BOOL)setEnabled:(BOOL)enabled keepDisplayAwake:(BOOL)keepDisplayAwake error:(NSError **)error;
@end
