$ErrorActionPreference = "Stop"

$versionHeader = Get-Content "src/Version.h" -Raw
function Read-VersionDefine([string]$name) {
    $match = [regex]::Match($versionHeader, "(?m)^#define\s+$name\s+(\d+)$")
    if (-not $match.Success) { throw "Unable to read $name from src/Version.h" }
    return [int]$match.Groups[1].Value
}

$major = Read-VersionDefine "VER_MAJOR"
$minor = Read-VersionDefine "VER_MINOR"
$patch = Read-VersionDefine "VER_PATCH"
$sourceVersion = "$major.$minor.$patch"
$releaseVersion = if ($patch -eq 0) { "$major.$minor" } else { $sourceVersion }

$cmake = Get-Content "CMakeLists.txt" -Raw
if ($cmake -match 'project\(Everon VERSION [0-9]') {
    throw "CMakeLists.txt must derive the project version from src/Version.h"
}

foreach ($readme in @("README.md", "README.ru.md")) {
    $content = Get-Content $readme -Raw
    if ($content -notmatch [regex]::Escape($sourceVersion)) {
        throw "$readme does not contain source version $sourceVersion"
    }
}

foreach ($changelog in @("CHANGELOG.md", "CHANGELOG.ru.md")) {
    $content = Get-Content $changelog -Raw
    if ($content -notmatch "(?m)^##\s+$([regex]::Escape($sourceVersion))\b") {
        throw "$changelog does not contain version $sourceVersion"
    }
}

Write-Host "Version consistency verified: source=$sourceVersion release=$releaseVersion"
