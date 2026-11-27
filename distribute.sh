#!/bin/bash
set -euo pipefail

# ============================================================================
# De-Sipper - macOS Distribution Build
# Universal binary -> codesign -> notarize -> PACE AAX sign -> stage dist
# Then: ./scripts/build_installer.sh --from-dist [--demo]
# ============================================================================
# Usage: ./distribute.sh [version] [--demo]
#   version defaults to project(Sipper VERSION ...) and must match it.
#   --demo builds with -DBUILD_DEMO=ON into build-demo/ and stages
#   dist/desipper-v<version>-demo.
#
# PACE: set PACE_WCGUID to De-Sipper's wrap config GUID before running.
# It is deliberately not hardcoded: the GUIDs in pour/distribute.sh and
# ontap/distribute.sh disagree about which one is "shared", and a wrong GUID
# signs fine here but is rejected by Pro Tools on other Macs.
# ============================================================================

PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
CMAKE_VERSION="$(sed -n 's/^project(Sipper VERSION \([0-9.]*\).*/\1/p' "$PROJECT_DIR/CMakeLists.txt")"

VERSION=""; DEMO=0
for a in "$@"; do
    case "$a" in
        --demo) DEMO=1;;
        -*) echo "unknown arg: $a"; exit 1;;
        *) VERSION="$a";;
    esac
done
VERSION="${VERSION:-$CMAKE_VERSION}"
if [ "$VERSION" != "$CMAKE_VERSION" ]; then
    echo "ERROR: version $VERSION does not match CMakeLists.txt ($CMAKE_VERSION)."
    exit 1
fi

: "${PACE_WCGUID:?set PACE_WCGUID to the De-Sipper PACE wrap config GUID}"

if [ "$DEMO" = 1 ]; then
    BUILD_DIR="$PROJECT_DIR/build-demo"; DEMO_FLAG="ON"; DIST_DIR="$PROJECT_DIR/dist/desipper-v${VERSION}-demo"
else
    BUILD_DIR="$PROJECT_DIR/build-release"; DEMO_FLAG="OFF"; DIST_DIR="$PROJECT_DIR/dist/desipper-v${VERSION}"
fi
ARTEFACTS="$BUILD_DIR/Sipper_artefacts/Release"

SIGN_ID="45C26CF1655F48EBC8A21802BDA053514719E1F0"
NOTARY_PROFILE="carbonator-notary"
WRAPTOOL="/Applications/PACEAntiPiracy/Eden/Fusion/Versions/5/bin/wraptool"
PACE_ACCOUNT="sodanswishers"

VST3="$ARTEFACTS/VST3/Sipper.vst3"
AU="$ARTEFACTS/AU/Sipper.component"
AAX="$ARTEFACTS/AAX/Sipper.aaxplugin"
AAX_SIGNED="$ARTEFACTS/AAX/Sipper_signed.aaxplugin"
STANDALONE="$ARTEFACTS/Standalone/Sipper.app"

echo "=== De-Sipper v${VERSION}$( [ "$DEMO" = 1 ] && echo ' DEMO' || true ) - Distribution Build ==="
echo ""

# --- Step 1: Universal build ---
echo "[1/6] Building universal binary (arm64 + x86_64), BUILD_DEMO=$DEMO_FLAG..."
cd "$PROJECT_DIR"
cmake -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" \
    -DCMAKE_OSX_DEPLOYMENT_TARGET="10.13" \
    -DCOPY_AFTER_BUILD=OFF \
    -DBUILD_DEMO="$DEMO_FLAG" > /dev/null
cmake --build "$BUILD_DIR" --target Sipper_VST3 Sipper_AU Sipper_AAX Sipper_Standalone \
    --config Release --clean-first 2>&1 | tail -1

echo "  Verifying architectures..."
for bin in "$VST3/Contents/MacOS/Sipper" "$AU/Contents/MacOS/Sipper" \
           "$STANDALONE/Contents/MacOS/Sipper" "$AAX/Contents/MacOS/Sipper"; do
    ARCH=$(lipo -info "$bin" 2>/dev/null | grep -o "x86_64 arm64" || true)
    if [ "$ARCH" != "x86_64 arm64" ]; then
        echo "  ERROR: $bin is not universal!"
        exit 1
    fi
done
echo "  All binaries are universal."

echo "  Setting LSMinimumSystemVersion=10.13 in all Info.plists..."
for plist in "$VST3/Contents/Info.plist" "$AU/Contents/Info.plist" \
             "$AAX/Contents/Info.plist" "$STANDALONE/Contents/Info.plist"; do
    /usr/libexec/PlistBuddy -c "Delete :LSMinimumSystemVersion" "$plist" 2>/dev/null || true
    /usr/libexec/PlistBuddy -c "Add :LSMinimumSystemVersion string 10.13" "$plist"
done
echo ""

# --- Step 2: Code sign VST3, AU, Standalone ---
echo "[2/6] Code signing VST3, AU, Standalone..."
for plugin in "$VST3" "$AU" "$STANDALONE"; do
    codesign --force --deep --options runtime --timestamp --sign "$SIGN_ID" "$plugin"
    echo "  Signed: $(basename "$plugin")"
done
echo ""

# --- Step 3: Notarize ---
echo "[3/6] Submitting for Apple notarization..."
NOTARY_PIDS=()
for plugin in "$VST3" "$AU" "$STANDALONE"; do
    ZIP="$(mktemp -d)/$(basename "$plugin").zip"
    ditto -c -k --keepParent "$plugin" "$ZIP"
    echo "  Submitting $(basename "$plugin")..."
    xcrun notarytool submit "$ZIP" --keychain-profile "$NOTARY_PROFILE" --wait 2>&1 | tail -2 &
    NOTARY_PIDS+=($!)
done
NOTARY_FAILED=0
for pid in "${NOTARY_PIDS[@]}"; do wait "$pid" || NOTARY_FAILED=1; done
if [ "$NOTARY_FAILED" -eq 1 ]; then
    echo "  ERROR: notarization failed."
    echo "  Run: xcrun notarytool log <submission-id> --keychain-profile $NOTARY_PROFILE"
    exit 1
fi
echo ""

# --- Step 4: Staple ---
echo "[4/6] Stapling notarization tickets..."
for plugin in "$VST3" "$AU" "$STANDALONE"; do
    xcrun stapler staple "$plugin" > /dev/null
    echo "  Stapled: $(basename "$plugin")"
done
echo ""

# --- Step 5: PACE sign AAX ---
echo "[5/6] Signing AAX with PACE wraptool..."
read -rsp "  PACE password for '$PACE_ACCOUNT': " PACE_PASSWORD
echo ""
rm -rf "$AAX_SIGNED"
"$WRAPTOOL" sign \
    --verbose \
    --account "$PACE_ACCOUNT" \
    --password "$PACE_PASSWORD" \
    --wcguid "$PACE_WCGUID" \
    --signid "$SIGN_ID" \
    --in "$AAX" \
    --out "$AAX_SIGNED" 2>&1 | tail -5
[ -d "$AAX_SIGNED" ] || { echo "  ERROR: wraptool produced no signed AAX"; exit 1; }
echo ""

# --- Step 6: Stage dist (ditto keeps the PACE symlinks intact) ---
echo "[6/6] Staging distribution..."
rm -rf "$DIST_DIR"
mkdir -p "$DIST_DIR"/{VST3,AU,AAX,Standalone}
ditto "$VST3"       "$DIST_DIR/VST3/Sipper.vst3"
ditto "$AU"         "$DIST_DIR/AU/Sipper.component"
ditto "$AAX_SIGNED" "$DIST_DIR/AAX/Sipper.aaxplugin"
# The shipped v1.0.0 installed the app as De-Sipper.app; keep that name.
ditto "$STANDALONE" "$DIST_DIR/Standalone/De-Sipper.app"
echo "  $DIST_DIR"
echo ""

echo "=== Verification ==="
for plugin in "$DIST_DIR/VST3/Sipper.vst3" "$DIST_DIR/AU/Sipper.component" \
              "$DIST_DIR/Standalone/De-Sipper.app" "$DIST_DIR/AAX/Sipper.aaxplugin"; do
    AUTH=$(codesign -dvv "$plugin" 2>&1 | grep "Authority=Developer" | head -1 | sed 's/Authority=//')
    printf "  %-30s %s\n" "$(basename "$plugin")" "$AUTH"
done
for plugin in "$DIST_DIR/VST3/Sipper.vst3" "$DIST_DIR/AU/Sipper.component" "$DIST_DIR/Standalone/De-Sipper.app"; do
    codesign --verify --deep --strict "$plugin" || { echo "  ERROR: signature broken on $(basename "$plugin")"; exit 1; }
    RESULT=$(xcrun stapler validate "$plugin" 2>&1 | grep -o "worked" || echo "FAILED")
    printf "  %-30s staple %s\n" "$(basename "$plugin")" "$RESULT"
done
echo ""
echo "Next: ./scripts/build_installer.sh --from-dist$( [ "$DEMO" = 1 ] && echo ' --demo' || true )"
