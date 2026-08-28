$ErrorActionPreference = "Stop"

$vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $vswhere)) {
    throw "vswhere.exe was not found"
}

$installation = (& $vswhere -latest -products * -property installationPath).Trim()
if (-not $installation) {
    throw "Visual Studio installation was not found"
}

$coverageTool = Join-Path $installation "Common7\IDE\Extensions\Microsoft\CodeCoverage.Console\Microsoft.CodeCoverage.Console.exe"
if (-not (Test-Path $coverageTool)) {
    throw "Microsoft.CodeCoverage.Console.exe was not found"
}

$coverageDir = Join-Path $PWD "coverage"
Remove-Item $coverageDir -Recurse -Force -ErrorAction SilentlyContinue
New-Item $coverageDir -ItemType Directory | Out-Null

$tests = @(
    "TimerModeTests",
    "HotkeyManagerTests",
    "SettingsTests",
    "PowerManagerTests",
    "PowerPolicyTests",
    "LocalizationTests",
    "OwnershipTests",
    "SingleInstanceTests"
)

foreach ($test in $tests) {
    $executable = Join-Path $PWD "build\Release\$test.exe"
    if (-not (Test-Path $executable)) {
        throw "Coverage test executable was not found: $executable"
    }

    $output = Join-Path $coverageDir "$test.coverage"
    & $coverageTool collect -o $output -f coverage $executable
    if ($LASTEXITCODE -ne 0) {
        throw "Coverage collection failed for $test"
    }
}

$inputs = @(Get-ChildItem $coverageDir -Filter *.coverage | ForEach-Object { $_.FullName })
if ($inputs.Count -eq 0) {
    throw "No coverage files were produced"
}

$report = Join-Path $coverageDir "coverage.xml"
& $coverageTool merge @inputs -o $report -f xml
if ($LASTEXITCODE -ne 0 -or -not (Test-Path $report)) {
    throw "Coverage merge failed"
}

[xml]$coverageXml = Get-Content $report -Raw
$modules = @($coverageXml.SelectNodes("//*[local-name()='module']"))
if ($modules.Count -eq 0) {
    throw "Coverage report contains no instrumented modules"
}

$functions = @($coverageXml.SelectNodes("//*[local-name()='function']"))
Write-Host "Coverage report: $report ($($modules.Count) modules, $($functions.Count) functions)"
