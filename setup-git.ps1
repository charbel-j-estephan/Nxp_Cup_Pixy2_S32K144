# setup-git.ps1
# Run this once to initialize git and push to GitHub
# Usage: Right-click -> Run with PowerShell, or open in terminal and run:
#   Set-ExecutionPolicy Bypass -Scope Process; .\setup-git.ps1

$ErrorActionPreference = "Stop"
$ProjectPath = $PSScriptRoot

Write-Host "=== NXP Cup Git Setup ===" -ForegroundColor Cyan
Write-Host "Project: $ProjectPath"

# Remove any broken .git directory from failed init attempt
if (Test-Path "$ProjectPath\.git") {
    Write-Host "Removing existing (broken) .git folder..." -ForegroundColor Yellow
    Remove-Item -Recurse -Force "$ProjectPath\.git"
}

# Initialize git
Write-Host "`nInitializing git repository..." -ForegroundColor Green
git -C "$ProjectPath" init -b main

# Set identity (update these if needed)
git -C "$ProjectPath" config user.email "charbelstephan13@gmail.com"
git -C "$ProjectPath" config user.name "Charbel"

# Stage all non-ignored files
Write-Host "`nStaging files..." -ForegroundColor Green
git -C "$ProjectPath" add .

# Show what will be committed
Write-Host "`nFiles to be committed:" -ForegroundColor Cyan
git -C "$ProjectPath" status --short

# Make initial commit
Write-Host "`nMaking initial commit..." -ForegroundColor Green
git -C "$ProjectPath" commit -m "Initial commit: NXP Cup Pixy2 S32K144 project"

Write-Host "`n=== Git init done! ===" -ForegroundColor Green
git -C "$ProjectPath" log --oneline

# --- GitHub section ---
Write-Host "`n=== GitHub Push ===" -ForegroundColor Cyan

$ghAvailable = $null -ne (Get-Command gh -ErrorAction SilentlyContinue)

if ($ghAvailable) {
    Write-Host "gh CLI found. Checking auth..." -ForegroundColor Green
    gh auth status

    Write-Host "`nCreating private GitHub repo 'Nxp_Cup_Pixy2_S32K144'..." -ForegroundColor Green
    gh repo create Nxp_Cup_Pixy2_S32K144 --private --source="$ProjectPath" --remote=origin --push

    Write-Host "`nAll done! Repo URL:" -ForegroundColor Green
    gh repo view --json url -q .url
} else {
    Write-Host "gh CLI not found. Please run these commands manually:" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "  # Create repo on GitHub (via browser or gh CLI):"
    Write-Host "  # Then add remote and push:"
    Write-Host "  git -C `"$ProjectPath`" remote add origin https://github.com/YOUR_USERNAME/Nxp_Cup_Pixy2_S32K144.git"
    Write-Host "  git -C `"$ProjectPath`" push -u origin main"
}

Write-Host "`nPress Enter to exit..."
Read-Host
