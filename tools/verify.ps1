param(
    [string]$CMakePath = $env:QIANSAI_CMAKE,
    [string]$NinjaPath = $env:QIANSAI_NINJA,
    [switch]$Clean
)

$ErrorActionPreference = "Stop"

$Root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$VerifyRoot = Join-Path $Root "build\verify"
$ToolchainFile = Join-Path $Root "gcc-arm-none-eabi.cmake"

function Find-Tool {
    param(
        [string]$Name,
        [string]$ConfiguredPath
    )

    if ($ConfiguredPath -and (Test-Path $ConfiguredPath)) {
        return (Resolve-Path $ConfiguredPath).Path
    }

    $command = Get-Command $Name -ErrorAction SilentlyContinue
    if ($null -ne $command) {
        return $command.Source
    }

    throw "Unable to find $Name. Add STM32CubeCLT to PATH or set the script parameter."
}

function Invoke-CMake {
    param(
        [string]$Executable,
        [string[]]$Arguments
    )

    $PreviousErrorActionPreference = $ErrorActionPreference
    $ErrorActionPreference = "Continue"
    & $Executable @Arguments 2>&1 | ForEach-Object { Write-Host $_ }
    $ErrorActionPreference = $PreviousErrorActionPreference
    if ($LASTEXITCODE -ne 0) {
        throw "CMake failed with exit code $LASTEXITCODE."
    }
}

function Invoke-CoreBuild {
    param(
        [string]$Core,
        [string]$ProjectDir,
        [string]$ArtifactName
    )

    $BuildDir = Join-Path $VerifyRoot $Core
    New-Item -ItemType Directory -Path $BuildDir -Force | Out-Null

    Invoke-CMake $CMake @(
        "-S", $ProjectDir,
        "-B", $BuildDir,
        "-G", "Ninja",
        "-DCMAKE_BUILD_TYPE=Debug",
        "-DCMAKE_TOOLCHAIN_FILE:FILEPATH=$ToolchainFile",
        "-DCMAKE_MAKE_PROGRAM:FILEPATH=$Ninja"
    )

    $BuildArguments = @("--build", $BuildDir)
    if ($Clean) {
        $BuildArguments += "--clean-first"
    }
    Invoke-CMake $CMake $BuildArguments

    $Artifact = Join-Path $BuildDir $ArtifactName
    if (-not (Test-Path $Artifact)) {
        throw "Missing firmware artifact: $Artifact"
    }

    return (Resolve-Path $Artifact).Path
}

$CMake = Find-Tool "cmake" $CMakePath
$Ninja = Find-Tool "ninja" $NinjaPath

if (-not (Test-Path $ToolchainFile)) {
    throw "Missing toolchain file: $ToolchainFile"
}

if ($Clean -and (Test-Path $VerifyRoot)) {
    $FullRoot = (Resolve-Path $Root).Path
    $FullVerifyRoot = (Resolve-Path $VerifyRoot).Path
    $ExpectedPrefix = $FullRoot + [System.IO.Path]::DirectorySeparatorChar
    if (-not $FullVerifyRoot.StartsWith($ExpectedPrefix, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to clean a directory outside the repository: $FullVerifyRoot"
    }

    Remove-Item -LiteralPath $FullVerifyRoot -Recurse -Force
}

New-Item -ItemType Directory -Path $VerifyRoot -Force | Out-Null

$TrackedFiles = & git -c safe.directory=* ls-files
if ($LASTEXITCODE -ne 0) {
    throw "Unable to list Git-tracked files."
}

$SharedHeaders = @(
    $TrackedFiles | Where-Object { $_ -eq "Common/Inc/shared_buf.h" -or $_ -eq "Common/Inc/shared_config.h" }
)
if ($SharedHeaders.Count -ne 2) {
    throw "Expected exactly two shared headers under Common/Inc."
}

$DuplicateSharedHeaders = @(
    $TrackedFiles | Where-Object { $_ -match "shared_(buf|config)\.h$" -and $_ -notmatch "^Common/Inc/" }
)
if ($DuplicateSharedHeaders.Count -ne 0) {
    throw "Duplicate shared headers found: $($DuplicateSharedHeaders -join ', ')"
}

$RequiredModuleSources = @(
    "CM4/Modules/Protocols/UART/my_uart_check.c",
    "CM4/Modules/Protocols/SPI/my_spi_check.c",
    "CM4/Modules/Protocols/I2C/my_i2c_check.c",
    "CM4/Modules/Protocols/CAN/my_can_check.c",
    "CM4/Modules/Protocols/Timing/my_dwt_count.c"
)

foreach ($RelativePath in $RequiredModuleSources) {
    if (-not (Test-Path (Join-Path $Root $RelativePath))) {
        throw "Missing required module source: $RelativePath"
    }
}

$CM4Elf = Invoke-CoreBuild "CM4" (Join-Path $Root "CM4") "qiansai_CM4.elf"
$CM7Elf = Invoke-CoreBuild "CM7" (Join-Path $Root "CM7") "qiansai_CM7.elf"

$CM4CompileCommandsPath = Join-Path $VerifyRoot "CM4\compile_commands.json"
$CM4CompileCommands = Get-Content $CM4CompileCommandsPath -Raw | ConvertFrom-Json
foreach ($RelativePath in $RequiredModuleSources) {
    $ExpectedSuffix = $RelativePath.Replace("\", "/")
    $Match = @(
        $CM4CompileCommands | Where-Object {
            $_.file.Replace("\", "/").EndsWith($ExpectedSuffix, [System.StringComparison]::OrdinalIgnoreCase)
        }
    )

    if ($Match.Count -eq 0) {
        throw "Module source was not compiled from its new location: $RelativePath"
    }
}

Write-Host "Shared contracts: OK"
Write-Host "CM4 modules: OK"
Write-Host "CM4 firmware: $CM4Elf"
Write-Host "CM7 firmware: $CM7Elf"
