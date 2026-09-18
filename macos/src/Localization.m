#import "Localization.h"

@implementation Localization

static NSBundle *languageBundle;

+ (NSArray<NSString *> *)supportedLanguageCodes {
    return @[@"en", @"ru", @"fr", @"de", @"it", @"es"];
}

+ (NSString *)resolvedLanguageCode:(NSString *)code {
    if ([code isEqualToString:@"system"] || code.length == 0) {
        NSString *preferred = NSLocale.preferredLanguages.firstObject ?: @"en";
        NSString *prefix = [[preferred componentsSeparatedByString:@"-"] firstObject].lowercaseString;
        return [[self supportedLanguageCodes] containsObject:prefix] ? prefix : @"en";
    }
    return [[self supportedLanguageCodes] containsObject:code] ? code : @"en";
}

+ (void)reloadBundle {
    NSString *code = [self resolvedLanguageCode:[self languageCode]];
    NSString *path = [NSBundle.mainBundle pathForResource:code ofType:@"lproj"];
    languageBundle = path ? [NSBundle bundleWithPath:path] : NSBundle.mainBundle;
}

+ (NSString *)languageCode {
    return [NSUserDefaults.standardUserDefaults stringForKey:@"Language"] ?: @"system";
}

+ (void)setLanguageCode:(NSString *)code {
    [NSUserDefaults.standardUserDefaults setObject:code forKey:@"Language"];
    [self reloadBundle];
}

+ (NSString *)string:(NSString *)key {
    if (!languageBundle) {
        [self reloadBundle];
    }
    return [languageBundle localizedStringForKey:key value:key table:nil];
}

@end
