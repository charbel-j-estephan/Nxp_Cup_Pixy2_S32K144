#!/bin/bash
set -e
cd /c/Users/Charbel/workspaceS32DS.3.6.5/Nxp_Cup_Pixy2_S32K144
gh repo create Nxp_Cup_Pixy2_S32K144 --private --source=. --remote=origin --push
echo "=== Push done ==="
gh repo view --json url -q .url
