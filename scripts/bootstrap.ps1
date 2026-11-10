#Requires -Version 5.1
<#
.SYNOPSIS
    Fetches the third-party sources NiiX Torrent is built from on Windows.

.DESCRIPTION
    Everything is pinned to an exact release so builds are reproducible. To
    update a dependency, change its version below (and, for the libraries that
    come from vcpkg, the "builtin-baseline" in vcpkg.json), then run this
    script again with -Force.

      wxWidgets   3.3.4        UI toolkit                   vendor/wx
      vcpkg       2026.10.16   libtorrent 2.1, Boost 1.92,  vendor/vcpkg
                               fmt 12, nlohmann-json 3.12,
                               OpenSSL 3.6, SQLite
      ANTLR       4.13.2       filter language runtime      vendor/antlr4
      Crashpad    (optional)   crash reporter               vendor/crashpad
#>
[CmdletBinding()]
param(
    # Re-download dependencies that are already present.
    [switch] $Force,

    # Also fetch Crashpad (crash reporting is off unless this is present).
    [switch] $WithCrashpad
)

$ErrorActionPreference = "Stop"

$Root      = Split-Path -Parent $PSScriptRoot
$VendorDir = Join-Path $Root "vendor"

function Get-Dependency {
    param(
        [Parameter(Mandatory)] [string] $Name,
        [Parameter(Mandatory)] [string] $Url,
        [Parameter(Mandatory)] [string] $Ref,
        [switch] $Recursive,
        [switch] $Shallow       # $Ref must be a tag or branch name
    )

    $target = Join-Path $VendorDir $Name
    $marker = Join-Path $target ".niix-ref"

    if ((Test-Path $marker) -and ((Get-Content $marker -TotalCount 1) -eq $Ref) -and -not $Force) {
        Write-Host "[skip] vendor/$Name is at $Ref" -ForegroundColor DarkGray
        return
    }

    if (Test-Path $target) {
        Write-Host "[update] vendor/$Name -> $Ref" -ForegroundColor Cyan
        Remove-Item -Recurse -Force $target
    }
    else {
        Write-Host "[clone] vendor/$Name ($Ref)" -ForegroundColor Cyan
    }

    if ($Shallow) {
        # Only the files of this one release, without the project's history -
        # a fraction of the download and of the disk space.
        git -c advice.detachedHead=false clone --quiet --depth 1 --branch $Ref $Url $target
        if ($LASTEXITCODE -ne 0) { throw "Could not clone $Url ($Ref)" }
    }
    else {
        git clone --quiet $Url $target
        if ($LASTEXITCODE -ne 0) { throw "Could not clone $Url" }

        git -C $target -c advice.detachedHead=false checkout --quiet $Ref
        if ($LASTEXITCODE -ne 0) { throw "Could not check out $Ref in vendor/$Name" }
    }

    if ($Recursive) {
        git -C $target submodule update --init --recursive --depth 1
        if ($LASTEXITCODE -ne 0) { throw "Could not fetch the submodules of vendor/$Name" }
    }

    Set-Content -Path $marker -Value $Ref
}

if (-not (Get-Command git -ErrorAction SilentlyContinue)) {
    throw "git is required but was not found on PATH."
}

New-Item -ItemType Directory -Force -Path $VendorDir | Out-Null

Get-Dependency -Name "wx"     -Url "https://github.com/wxWidgets/wxWidgets" -Ref "v3.3.4"     -Recursive -Shallow
Get-Dependency -Name "vcpkg"  -Url "https://github.com/microsoft/vcpkg"     -Ref "2026.10.16"
Get-Dependency -Name "antlr4" -Url "https://github.com/antlr/antlr4"        -Ref "4.13.2"     -Shallow

if ($WithCrashpad) {
    # The maintained CMake-enabled fork of Chromium's Crashpad.
    Get-Dependency -Name "crashpad" -Url "https://github.com/getsentry/crashpad" `
        -Ref "702c3d5a0d02c9b72a8b029d33bdfcd81dc75f65" -Recursive
}

# ---- Patches for vendored sources -------------------------------------------
# ANTLR 4.13.2's ProfilingATNSimulator.cpp uses std::chrono::high_resolution_clock
# without including <chrono>, which current MSVC no longer pulls in
# transitively. Add the include (idempotent).
$antlrProfiler = Join-Path $VendorDir "antlr4/runtime/Cpp/runtime/src/atn/ProfilingATNSimulator.cpp"
if (Test-Path $antlrProfiler) {
    $content = Get-Content -Raw $antlrProfiler
    if ($content -notmatch '#include\s*<chrono>') {
        Write-Host "[patch] adding <chrono> to ANTLR ProfilingATNSimulator.cpp" -ForegroundColor Cyan
        Set-Content -Path $antlrProfiler -Value ("#include <chrono>`r`n" + $content) -NoNewline
    }
}
# -----------------------------------------------------------------------------

$vcpkgExe = Join-Path $VendorDir "vcpkg/vcpkg.exe"
if (-not (Test-Path $vcpkgExe)) {
    Write-Host "[vcpkg] bootstrapping" -ForegroundColor Cyan
    & (Join-Path $VendorDir "vcpkg/bootstrap-vcpkg.bat") -disableMetrics
    if ($LASTEXITCODE -ne 0) { throw "vcpkg could not be bootstrapped." }
}

Write-Host ""
Write-Host "All dependencies are ready. Next:  .\build.ps1" -ForegroundColor Green
