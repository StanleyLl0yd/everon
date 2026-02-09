#pragma once

// Single source of truth for Everon version.
// Keep this file in sync with the released version number.

#define VER_MAJOR 2
#define VER_MINOR 4
#define VER_PATCH 0
#define VER_BUILD 0

// For VS_VERSION_INFO numeric fields
#define VER_FILEVERSION        VER_MAJOR,VER_MINOR,VER_PATCH,VER_BUILD
#define VER_PRODUCTVERSION     VER_FILEVERSION

// For VS_VERSION_INFO string fields
#define VER_FILEVERSION_STR    "2.4.0.0"
#define VER_PRODUCTVERSION_STR VER_FILEVERSION_STR

// For UI (About, etc.)
#define VER_VERSION_STR        "2.4"
#define VER_VERSION_STR_W      L"2.4"
#define VER_FILEVERSION_STR_W  L"2.4.0.0"
