#!/bin/bash
# run_make.sh  –  Build from Git Bash; output to build_output.txt
set -o pipefail

PROJECT_ROOT="/c/Users/Charbel/workspaceS32DS.3.6.5/Nxp_Cup_Pixy2_S32K144"
BUILD_DIR="$PROJECT_ROOT/Debug_FLASH"
OUTPUT="$PROJECT_ROOT/build_output.txt"

echo "=== NXP Cup build $(date) ===" | tee "$OUTPUT"

# Locate arm-none-eabi-gcc inside S32DS (10.2 toolchain preferred)
GCC_BIN=$(find /c/NXP/S32DS.3.6.5 -name "arm-none-eabi-gcc.exe" 2>/dev/null \
           | grep -i "10.2\|10_2" | head -1 | xargs -I{} dirname {} 2>/dev/null)
if [ -z "$GCC_BIN" ]; then
    GCC_BIN=$(find /c/NXP/S32DS.3.6.5 -name "arm-none-eabi-gcc.exe" 2>/dev/null \
              | head -1 | xargs -I{} dirname {} 2>/dev/null)
fi
if [ -z "$GCC_BIN" ]; then
    echo "ERROR: arm-none-eabi-gcc not found in C:\NXP\S32DS.3.6.5" | tee -a "$OUTPUT"
    exit 1
fi
echo "GCC  : $GCC_BIN" | tee -a "$OUTPUT"
export PATH="$GCC_BIN:$PATH"

# Locate make.exe inside S32DS MinGW utilities
MAKE_EXE=$(find /c/NXP/S32DS.3.6.5 -name "make.exe" 2>/dev/null \
            | grep -v cygwin | head -1)
if [ -z "$MAKE_EXE" ]; then
    MAKE_EXE=$(which make 2>/dev/null)
fi
if [ -z "$MAKE_EXE" ]; then
    echo "ERROR: make not found" | tee -a "$OUTPUT"
    exit 1
fi
echo "Make : $MAKE_EXE" | tee -a "$OUTPUT"
echo "" | tee -a "$OUTPUT"

# Build (do NOT run make clean – it deletes the Eclipse-generated Makefile itself,
# which cannot be recovered without the IDE. Use make all for incremental builds.)
cd "$BUILD_DIR" || exit 1
if [ ! -f "makefile" ] && [ ! -f "Makefile" ]; then
    echo "ERROR: No Makefile found in $BUILD_DIR" | tee -a "$OUTPUT"
    echo "  --> Please build once from S32DS IDE (Project > Build Project)" | tee -a "$OUTPUT"
    echo "      to regenerate the Eclipse CDT makefiles, then run this script again." | tee -a "$OUTPUT"
    exit 1
fi
echo "=== make all ===" | tee -a "$OUTPUT"
"$MAKE_EXE" all 2>&1 | tee -a "$OUTPUT"
BUILD_STATUS=${PIPESTATUS[0]}

echo ""  | tee -a "$OUTPUT"
echo "=== Build finished, exit $BUILD_STATUS ===" | tee -a "$OUTPUT"
exit $BUILD_STATUS
