add_executable(Everon MACOSX_BUNDLE
    src/main.m
    src/AppDelegate.m
    src/PowerManager.m
    src/HotkeyManager.m
    src/Localization.m
)

target_include_directories(Everon PRIVATE src)
target_compile_options(Everon PRIVATE -Wall -Wextra -Wpedantic -Wno-deprecated-declarations -fobjc-arc)
if(EVERON_WARNINGS_AS_ERRORS)
    target_compile_options(Everon PRIVATE -Werror)
endif()

target_link_libraries(Everon PRIVATE
    "-framework AppKit"
    "-framework Foundation"
    "-framework IOKit"
    "-framework ServiceManagement"
    "-framework UserNotifications"
    "-framework CoreGraphics"
    "-framework Carbon"
)

set_target_properties(Everon PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in"
    MACOSX_BUNDLE_BUNDLE_NAME "Everon"
    MACOSX_BUNDLE_GUI_IDENTIFIER "io.github.stanleyll0yd.everon"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
    MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
    OUTPUT_NAME "Everon"
)

add_custom_command(TARGET Everon POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/resources"
            "$<TARGET_BUNDLE_DIR:Everon>/Contents/Resources"
)
