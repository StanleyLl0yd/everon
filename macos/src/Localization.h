#pragma once

#import <Foundation/Foundation.h>

@interface Localization : NSObject
+ (NSString *)string:(NSString *)key;
+ (NSString *)languageCode;
+ (void)setLanguageCode:(NSString *)code;
+ (NSArray<NSString *> *)supportedLanguageCodes;
@end
