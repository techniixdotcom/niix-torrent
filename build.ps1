#Requires -Version 5.1
<#
.SYNOPSIS
    Builds the NiiX Torrent installer for Windows, or uploads the source code.

.DESCRIPTION
    Run it without arguments (or double-click build.cmd) and choose:

        1) Build the installer   -> dist\NiiXTorrent-<version>-x64-setup.exe
        2) Upload the source code to GitHub

    To skip the question:

        .\build.ps1 -Target Installer    # build the installer
        .\build.ps1 -Target Upload       # upload the source code
        .\build.ps1 -Target Clean        # remove build-* and dist

    Everything the build needs is downloaded and installed automatically:

      * Git, CMake, the Visual Studio C++ build tools and Inno Setup 7 -
        through winget, or straight from the vendors' download servers when
        winget is not available. Windows asks for administrator approval when
        an installer needs it.
      * wxWidgets, vcpkg and ANTLR sources - by scripts\bootstrap.ps1.
      * libtorrent, Boost, fmt, nlohmann/json, OpenSSL and SQLite - built by
        vcpkg during the CMake configure step.

    Pass -SkipInstall to only check for the tools and stop if one is missing.

.PARAMETER Platform
    x64 (default) or x86.

.PARAMETER WithCrashpad
    Also build the optional Crashpad crash reporter.
#>
[CmdletBinding()]
param(
    [ValidateSet("x64", "x86")]
    [string] $Platform = "x64",

    [ValidateSet("Installer", "Upload", "Clean")]
    [string] $Target,

    [ValidateSet("Release", "RelWithDebInfo", "Debug")]
    [string] $Configuration = "Release",

    [switch] $WithCrashpad,
    [switch] $SkipInstall
)

$ErrorActionPreference = "Stop"

$Root     = $PSScriptRoot
$BuildDir = Join-Path $Root "build-$Platform"
$DistRoot = Join-Path $Root "dist"
$DistDir  = Join-Path $DistRoot $Platform
$Version  = (Get-Content (Join-Path $Root "VERSION") -TotalCount 1).Trim()
$Triplet  = "$Platform-windows-static-md-rel"

Set-Location $Root

function Write-Step($m) { Write-Host "`n=== $m ===" -ForegroundColor Cyan }
function Write-Ok($m)   { Write-Host "  [ok] $m" -ForegroundColor Green }
function Write-Note($m) { Write-Host "  [..] $m" -ForegroundColor Yellow }

function Test-Cmd($name) {
    $null -ne (Get-Command $name -ErrorAction SilentlyContinue)
}

function Invoke-Native {
    # Runs an external program and stops the script if it fails.
    param([Parameter(Mandatory)] [string] $File, [string[]] $Arguments)

    & $File @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "'$File' failed with exit code $LASTEXITCODE."
    }
}

function Update-Path {
    $machine = [Environment]::GetEnvironmentVariable("Path", "Machine")
    $user    = [Environment]::GetEnvironmentVariable("Path", "User")
    $env:Path = (@($machine, $user) | Where-Object { $_ }) -join ';'
}

function Get-CMakeVersion {
    if (-not (Test-Cmd cmake)) { return $null }
    $line = & cmake --version 2>$null | Select-Object -First 1
    if ($line -match '(\d+)\.(\d+)') { return [version]("{0}.{1}" -f $Matches[1], $Matches[2]) }
    return $null
}

function Get-VSInstallPath {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
    if (-not (Test-Path $vswhere)) { return $null }

    $path = & $vswhere -latest -prerelease -products * `
        -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
        -property installationPath 2>$null

    if ($path) { return ($path | Select-Object -First 1) }
    return $null
}

function Get-InnoSetupCompiler {
    # packaging\windows\NiiXTorrent.iss needs Inno Setup 7 or newer; an older
    # ISCC.exe that happens to be on PATH does not count.
    $candidates = @()

    $cmd = Get-Command iscc -ErrorAction SilentlyContinue
    if ($cmd) { $candidates += $cmd.Source }

    $candidates += @(
        (Join-Path $env:ProgramFiles "Inno Setup 7\ISCC.exe"),
        (Join-Path ${env:ProgramFiles(x86)} "Inno Setup 7\ISCC.exe"),
        (Join-Path $env:LOCALAPPDATA "Programs\Inno Setup 7\ISCC.exe")
    )

    foreach ($candidate in $candidates) {
        if (-not ($candidate -and (Test-Path $candidate))) { continue }

        # ISCC.exe carries no version resource; its banner reads
        # "Inno Setup 7 Command-Line Compiler".
        $banner = (& $candidate "/?" 2>$null | Select-Object -First 1)
        if (($banner -match 'Inno Setup (\d+)') -and ([int]$Matches[1] -ge 7)) { return $candidate }
    }

    return $null
}

# Direct downloads used when winget is missing or does not deliver the tool.
$DirectDownloads = @{
    Git       = @{ Url  = "https://github.com/git-for-windows/git/releases/download/v2.56.0.windows.4/Git-2.56.0.4-64-bit.exe"
                   Args = @("/VERYSILENT", "/NORESTART", "/SUPPRESSMSGBOXES") }
    CMake     = @{ Url  = "https://github.com/Kitware/CMake/releases/download/v4.4.5/cmake-4.4.5-windows-x86_64.msi"
                   Args = @("/qn", "/norestart", "ADD_CMAKE_TO_PATH=System") }
    VSTools   = @{ Url  = "https://aka.ms/vs/17/release/vs_BuildTools.exe"
                   Args = @("--quiet", "--wait", "--norestart", "--nocache",
                            "--add", "Microsoft.VisualStudio.Workload.VCTools", "--includeRecommended") }
    InnoSetup = @{ Url  = "https://github.com/jrsoftware/issrc/releases/download/is-7_1_1/innosetup-7.1.1-x64.exe"
                   Args = @("/VERYSILENT", "/NORESTART", "/SUPPRESSMSGBOXES", "/ALLUSERS") }
}

function Install-WithWinget {
    param([Parameter(Mandatory)] [string] $Id, [string[]] $Extra)

    if (-not (Test-Cmd winget)) { return $false }

    $wingetArgs = @("install", "--id", $Id, "-e", "--accept-package-agreements", "--accept-source-agreements",
                    "--disable-interactivity")
    if ($Extra) { $wingetArgs += $Extra }

    Write-Note "winget $($wingetArgs -join ' ')"
    & winget @wingetArgs
    Update-Path

    # winget reports "already installed / no upgrade" with a non-zero code;
    # the caller re-checks for the tool, so only the attempt matters here.
    return $true
}

function Install-FromDownload {
    # Downloads an installer and runs it elevated (UAC prompt), waiting for it.
    param([Parameter(Mandatory)] [string] $Name)

    $entry = $DirectDownloads[$Name]
    $file  = Join-Path ([IO.Path]::GetTempPath()) ([IO.Path]::GetFileName(([Uri]$entry.Url).AbsolutePath))

    Write-Note "downloading $($entry.Url)"
    [Net.ServicePointManager]::SecurityProtocol = [Net.ServicePointManager]::SecurityProtocol -bor [Net.SecurityProtocolType]::Tls12
    $previous = $ProgressPreference
    $ProgressPreference = "SilentlyContinue"   # the progress bar makes Windows PowerShell downloads very slow
    try     { Invoke-WebRequest -Uri $entry.Url -OutFile $file -UseBasicParsing }
    finally { $ProgressPreference = $previous }

    Write-Note "installing $Name (approve the administrator prompt if one appears)"
    if ($file.ToLower().EndsWith(".msi")) {
        $process = Start-Process -FilePath "msiexec.exe" -ArgumentList (@("/i", "`"$file`"") + $entry.Args) -Verb RunAs -Wait -PassThru
    }
    else {
        $process = Start-Process -FilePath $file -ArgumentList $entry.Args -Verb RunAs -Wait -PassThru
    }

    # 3010 = success, restart required.
    if ($process.ExitCode -notin @(0, 3010)) {
        throw "The $Name installer failed with exit code $($process.ExitCode)."
    }

    Remove-Item -Force $file -ErrorAction SilentlyContinue
    Update-Path
}

function Install-Prerequisite {
    # Makes sure a tool is present: winget first, direct download as fallback.
    param(
        [Parameter(Mandatory)] [string] $Name,          # key in $DirectDownloads
        [Parameter(Mandatory)] [string] $Description,
        [Parameter(Mandatory)] [scriptblock] $Test,     # returns $true when the tool is usable
        [Parameter(Mandatory)] [string] $WingetId,
        [string[]] $WingetArgs
    )

    if (& $Test) { return }

    if ($SkipInstall) {
        throw "$Description is required but was not found (and -SkipInstall was given)."
    }

    Write-Note "$Description not found - installing"

    if (Install-WithWinget -Id $WingetId -Extra $WingetArgs) {
        if (& $Test) { return }
        Write-Note "winget did not provide a usable $Description - downloading it directly"
    }

    Install-FromDownload -Name $Name

    if (-not (& $Test)) {
        throw "$Description was installed but still cannot be found. Open a new terminal and run the script again."
    }
}

function Add-ToPathIfPresent($dir) {
    if ($dir -and (Test-Path $dir) -and (($env:Path -split ';') -notcontains $dir)) {
        $env:Path = "$dir;$env:Path"
    }
}

# ---------------------------------------------------------------------------
# What to do
# ---------------------------------------------------------------------------
if (-not $Target) {
    Write-Host ""
    Write-Host "NiiX Torrent $Version" -ForegroundColor Cyan
    Write-Host "  1) Build the installer  (dist\NiiXTorrent-$Version-$Platform-setup.exe)"
    Write-Host "  2) Upload the source code to GitHub"
    Write-Host ""

    do { $choice = (Read-Host "Choose 1 or 2").Trim() } until ($choice -in @("1", "2"))

    $Target = if ($choice -eq "1") { "Installer" } else { "Upload" }
}

if ($Target -eq "Upload") {
    $message = Read-Host "Describe the changes (press Enter for 'NiiX Torrent $Version')"
    $publish = Join-Path $Root "scripts\publish.ps1"

    if ($message.Trim()) { & $publish $message.Trim() }
    else                 { & $publish }
    return
}

# ---------------------------------------------------------------------------
# Clean
# ---------------------------------------------------------------------------
if ($Target -eq "Clean") {
    Write-Step "Cleaning"
    foreach ($dir in @((Join-Path $Root "build-x64"), (Join-Path $Root "build-x86"), $DistRoot)) {
        if (Test-Path $dir) {
            Remove-Item -Recurse -Force $dir
            Write-Ok "removed $dir"
        }
    }
    return
}


# ---------------------------------------------------------------------------
# 0. Disk space
# ---------------------------------------------------------------------------
# A first build downloads and compiles wxWidgets, Boost, libtorrent and OpenSSL
# from source; running out of space half-way gives confusing errors.
$RequiredFreeGB = 15

function Get-FreeSpaceGB($path) {
    try {
        $drive = New-Object System.IO.DriveInfo ([System.IO.Path]::GetPathRoot((Resolve-Path $path).Path))
        return [math]::Round($drive.AvailableFreeSpace / 1GB, 1)
    }
    catch { return $null }
}

$freeGB = Get-FreeSpaceGB $Root
if ($null -ne $freeGB) {
    $firstBuild = -not (Test-Path (Join-Path $BuildDir "vcpkg_installed"))
    if ($firstBuild -and ($freeGB -lt $RequiredFreeGB)) {
        throw ("Not enough free disk space: $freeGB GB free on the drive holding $Root, " +
               "but a first build needs about $RequiredFreeGB GB (plus about 8 GB on C: " +
               "if the Visual Studio build tools still have to be installed). " +
               "Free up space or move the project folder to a larger drive, then run the script again.")
    }
    elseif ($freeGB -lt 2) {
        throw "Not enough free disk space: only $freeGB GB free on the drive holding $Root."
    }
    Write-Host "Free disk space: $freeGB GB" -ForegroundColor DarkGray
}

# ---------------------------------------------------------------------------
# 1. Prerequisites
# ---------------------------------------------------------------------------
Write-Step "Checking prerequisites"

Install-Prerequisite -Name "Git" -Description "Git" -WingetId "Git.Git" -WingetArgs @("--silent") -Test {
    Add-ToPathIfPresent (Join-Path $env:ProgramFiles "Git\cmd")
    Test-Cmd git
}
Write-Ok "Git"

Install-Prerequisite -Name "CMake" -Description "CMake 3.28 or newer" -WingetId "Kitware.CMake" -WingetArgs @("--silent") -Test {
    Add-ToPathIfPresent (Join-Path $env:ProgramFiles "CMake\bin")
    $version = Get-CMakeVersion
    ($null -ne $version) -and ($version -ge [version]"3.28")
}
Write-Ok "CMake $(Get-CMakeVersion)"

Install-Prerequisite -Name "VSTools" -Description "Visual Studio C++ build tools" `
    -WingetId "Microsoft.VisualStudio.BuildTools" `
    -WingetArgs @("--override", "--quiet --wait --norestart --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended") `
    -Test { $null -ne (Get-VSInstallPath) }
$vsPath = Get-VSInstallPath
Write-Ok "Visual Studio C++ toolchain ($vsPath)"

Install-Prerequisite -Name "InnoSetup" -Description "Inno Setup 7" -WingetId "JRSoftware.InnoSetup" `
    -WingetArgs @("--silent") -Test { $null -ne (Get-InnoSetupCompiler) }
$iscc = Get-InnoSetupCompiler
Write-Ok "Inno Setup ($iscc)"

# ---------------------------------------------------------------------------
# 2. Third-party sources (wxWidgets, vcpkg, ANTLR, optionally Crashpad)
# ---------------------------------------------------------------------------
Write-Step "Fetching dependencies"

$bootstrapArgs = @{}
if ($WithCrashpad) { $bootstrapArgs["WithCrashpad"] = $true }
& (Join-Path $Root "scripts\bootstrap.ps1") @bootstrapArgs

# ---------------------------------------------------------------------------
# 3. Configure and build
# ---------------------------------------------------------------------------
Write-Step "Configuring ($Platform, $Configuration)"

$architecture = if ($Platform -eq "x64") { "x64" } else { "Win32" }

$configureArgs = @(
    "-S", $Root,
    "-B", $BuildDir,
    "-A", $architecture,
    "-DVCPKG_TARGET_TRIPLET=$Triplet",
    "-DVCPKG_HOST_TRIPLET=x64-windows-static-md-rel",
    # Delete each library's intermediate build files as soon as it is
    # installed; they are several gigabytes and never needed again.
    "-DVCPKG_INSTALL_OPTIONS=--clean-after-build"
)

# A configure run that was interrupted (for example by a full disk) leaves a
# broken cache behind; start that step from scratch.
$cache = Join-Path $BuildDir "CMakeCache.txt"
if ((Test-Path $cache) -and -not (Test-Path (Join-Path $BuildDir "NiiXTorrent.sln")) -and -not (Test-Path (Join-Path $BuildDir "NiiXTorrent.slnx"))) {
    Write-Note "removing the incomplete configuration from a previous run"
    Remove-Item -Force $cache
    Remove-Item -Recurse -Force (Join-Path $BuildDir "CMakeFiles") -ErrorAction SilentlyContinue
}

Invoke-Native "cmake" $configureArgs

# The libraries are installed in $BuildDir\vcpkg_installed (and cached per
# user) at this point. vcpkg's intermediate folders are only scratch space -
# several gigabytes of it - so reclaim them before compiling.
foreach ($scratch in @("buildtrees", "packages")) {
    $dir = Join-Path $Root "vendor\vcpkg\$scratch"
    if (Test-Path $dir) {
        Write-Note "freeing disk space: removing vendor\vcpkg\$scratch"
        Remove-Item -Recurse -Force $dir -ErrorAction SilentlyContinue
    }
}

Write-Step "Building"
Invoke-Native "cmake" @("--build", $BuildDir, "--config", $Configuration, "--parallel")

# ---------------------------------------------------------------------------
# 4. Stage the files that get shipped
# ---------------------------------------------------------------------------
Write-Step "Staging to $DistDir"

if (Test-Path $DistDir) { Remove-Item -Recurse -Force $DistDir }
Invoke-Native "cmake" @("--install", $BuildDir, "--config", $Configuration, "--prefix", $DistDir)
Write-Ok "application files staged"

# ---------------------------------------------------------------------------
# 5. Installer
# ---------------------------------------------------------------------------
Write-Step "Building the installer"

Invoke-Native $iscc @(
    "/Qp",
    "/DAppVersion=$Version",
    "/DArch=$Platform",
    "/DSourceDir=$DistDir",
    "/DOutputDir=$DistRoot",
    (Join-Path $Root "packaging\windows\NiiXTorrent.iss")
)

$setup = Join-Path $DistRoot "NiiXTorrent-$Version-$Platform-setup.exe"

Write-Host ""
Write-Host "Done. The installer is:" -ForegroundColor Green
Write-Host "  $setup"
