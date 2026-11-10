#Requires -Version 5.1
<#
.SYNOPSIS
    Uploads the source code to GitHub.

.DESCRIPTION
        .\scripts\publish.ps1                       # commit everything and push to main
        .\scripts\publish.ps1 "Linux port, v2.0"    # ... with your own commit message

    Works from a fresh unzip as well as from an existing clone. When the folder
    is not a git repository yet, it is connected to the GitHub repository first
    and your files are committed ON TOP of the history that is already there -
    nothing on GitHub is overwritten and no force-push is used.

    Build output and downloaded dependencies (build-*, dist, vendor\...) are
    excluded by .gitignore. Signing in to GitHub is handled by Git itself: the
    first push opens a browser window (Git Credential Manager).

.PARAMETER Message
    Commit message. Defaults to "NiiX Torrent <version>".

.PARAMETER Remote
    Repository URL.

.PARAMETER Branch
    Branch to push.
#>
[CmdletBinding()]
param(
    [Parameter(Position = 0)]
    [string] $Message,

    [string] $Remote = "https://github.com/techniixdotcom/niix-torrent.git",
    [string] $Branch = "main"
)

$ErrorActionPreference = "Stop"

$Root    = Split-Path -Parent $PSScriptRoot
$Version = (Get-Content (Join-Path $Root "VERSION") -TotalCount 1).Trim()
if (-not $Message) { $Message = "NiiX Torrent $Version" }

Set-Location $Root

function Invoke-Git {
    # Runs git and stops the script when it fails.
    & git @args
    if ($LASTEXITCODE -ne 0) { throw "git $($args -join ' ') failed (exit code $LASTEXITCODE)." }
}

function Test-Git {
    # Runs git quietly and reports success as $true / $false.
    $previous = $ErrorActionPreference
    $ErrorActionPreference = "Continue"
    try {
        & git @args 2>&1 | Out-Null
        return ($LASTEXITCODE -eq 0)
    }
    finally { $ErrorActionPreference = $previous }
}

if (-not (Get-Command git -ErrorAction SilentlyContinue)) {
    throw "Git was not found. Run .\build.ps1 once (it installs Git), open a new terminal and try again."
}

# Git needs to know who is committing.
$userName  = (& git config user.name)  2>$null
$userEmail = (& git config user.email) 2>$null
if (-not $userName -or -not $userEmail) {
    throw ("Git does not know your name and e-mail yet. Set them once with:`n" +
           "    git config --global user.name  `"Your Name`"`n" +
           "    git config --global user.email `"you@example.com`"")
}

if (-not (Test-Path (Join-Path $Root ".git"))) {
    Write-Host "[git] connecting this folder to $Remote" -ForegroundColor Cyan
    Invoke-Git init --quiet "--initial-branch=$Branch"
    Invoke-Git remote add origin $Remote

    if (Test-Git fetch --quiet origin $Branch) {
        # Adopt the existing history without touching the files in this
        # folder: they become the next commit on top of it.
        Invoke-Git reset --quiet --mixed FETCH_HEAD
        [void](Test-Git branch --quiet "--set-upstream-to=origin/$Branch" $Branch)
        Write-Host "[git] continuing from the existing history on GitHub" -ForegroundColor Cyan
    }
    else {
        Write-Host "[git] the repository has no '$Branch' branch yet - this will be its first commit" -ForegroundColor Yellow
    }
}
else {
    if (Test-Git remote get-url origin) { Invoke-Git remote set-url origin $Remote }
    else                                { Invoke-Git remote add origin $Remote }

    $current = (& git rev-parse --abbrev-ref HEAD).Trim()
    if ($current -ne $Branch) {
        throw "You are on branch '$current' but publishing '$Branch'. Switch with 'git switch $Branch' or pass -Branch $current."
    }
}

Invoke-Git add --all

# Keep the shell scripts executable for Linux checkouts - Windows has no
# executable bit, so it is recorded in the git index directly.
Invoke-Git update-index --chmod=+x build.sh scripts/bootstrap.sh scripts/prereqs.sh scripts/publish.sh

if (Test-Git diff --cached --quiet) {
    Write-Host "[git] no changes to commit" -ForegroundColor DarkGray
}
else {
    Write-Host "[git] changes in this upload:" -ForegroundColor Cyan
    & git diff --cached --shortstat
    Invoke-Git commit --quiet -m $Message
    Write-Host "[git] committed: $Message" -ForegroundColor Green
}

Write-Host "[git] pushing to $Remote ($Branch)" -ForegroundColor Cyan
& git push --set-upstream origin $Branch
if ($LASTEXITCODE -ne 0) {
    throw ("The push was rejected. If GitHub has commits you do not have here, run`n" +
           "    git pull --rebase origin $Branch`n" +
           "and then this script again. If it is a sign-in problem, check that your account may write to the repository.")
}

$web = $Remote -replace '\.git$', '' -replace '^git@github\.com:', 'https://github.com/'

Write-Host ""
Write-Host "Done: $web" -ForegroundColor Green
Write-Host "Create the release at: $web/releases/new"
