#!/bin/bash
set -e
P="/c/Users/Charbel/workspaceS32DS.3.6.5/Nxp_Cup_Pixy2_S32K144"
cd "$P"
echo "=== Removing broken .git ==="
rm -rf .git
echo "=== git init ==="
git init -b main
git config user.email "charbelstephan13@gmail.com"
git config user.name "Charbel"
echo "=== staging files ==="
git add .
echo "=== files staged: ==="
git status --short | head -30
echo "=== committing ==="
git commit -m "Initial commit: NXP Cup Pixy2 S32K144 project"
echo "=== commit done ==="
git log --oneline
echo "=== checking gh ==="
gh --version 2>/dev/null && echo "gh IS available" || echo "gh NOT found - push manually"
