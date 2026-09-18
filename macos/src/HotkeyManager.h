#pragma once

#import <Foundation/Foundation.h>

@interface HotkeyManager : NSObject
@property(nonatomic, copy) void (^handler)(void);
- (BOOL)registerPreset:(NSInteger)preset;
@end
