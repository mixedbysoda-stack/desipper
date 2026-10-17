#!/bin/bash
# build_installer.sh - macOS PKG: VST3 + AU + signed AAX (+ optional Standalone)
# -> productbuild wizard -> signed, notarized pkg.
#
# Same structure as TALLBOY's scripts/build_installer.sh. The Distribution
# declares hostArchitectures="arm64,x86_64" so Installer.app does not run under
# Rosetta on Apple Silicon (Q-57), and scripts/check_pkg_arch.sh gates both the
# unsigned and the final pkg so that cannot regress.
#
# Install paths match the shipped v1.0.0 pkg (Sipper.* bundles, De-Sipper.app),
# so an upgrade overwrites the existing install instead of adding a second copy.
#
# Usage (paid):
#   ./distribute.sh 1.0.1
#   ./scripts/build_installer.sh --from-dist
# Usage (demo):
#   ./distribute.sh 1.0.1 --demo
#   ./scripts/build_installer.sh --from-dist --demo
# Or point at bundles directly:
#   ./scripts/build_installer.sh [--demo] [--version X.Y.Z] \
#     --vst3 path/Sipper.vst3 --au path/Sipper.component \
#     --aax path/Sipper.aaxplugin [--app path/De-Sipper.app]
#
# --version defaults to project(Sipper VERSION ...) in CMakeLists.txt and must
# match it, so the pkg can never claim a version the binary does not report.
# The AAX input must already be PACE-signed (distribute.sh does that).
set -euo pipefail

INSTALLER_ID="Developer ID Installer: Miguel Silverio (K7FRP77ZZK)"
NOTARY_PROFILE="carbonator-notary"
PRODUCT="De-Sipper"
IDENT_ROOT="com.carbonatedaudio.desipper"
REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
RESOURCES="$REPO_ROOT/installer/Resources"
GATE="$REPO_ROOT/scripts/check_pkg_arch.sh"

CMAKE_VERSION="$(sed -n 's/^project(Sipper VERSION \([0-9.]*\).*/\1/p' "$REPO_ROOT/CMakeLists.txt")"
[[ -n "$CMAKE_VERSION" ]] || { echo "could not read project() VERSION from CMakeLists.txt"; exit 1; }

VERSION=""; VST3=""; AU=""; AAX=""; APP=""; DEMO=0; FROM_DIST=0
while [[ $# -gt 0 ]]; do
  case "$1" in
    --version)   VERSION="$2"; shift 2;;
    --vst3)      VST3="$2"; shift 2;;
    --au)        AU="$2";   shift 2;;
    --aax)       AAX="$2";  shift 2;;
    --app)       APP="$2";  shift 2;;
    --demo)      DEMO=1; shift;;
    --from-dist) FROM_DIST=1; shift;;
    *) echo "unknown arg: $1"; exit 1;;
  esac
done
VERSION="${VERSION:-$CMAKE_VERSION}"
[[ "$VERSION" == "$CMAKE_VERSION" ]] || {
  echo "--version $VERSION does not match CMakeLists.txt ($CMAKE_VERSION). Bump CMake first."; exit 1; }

SUFFIX=""; IDENT_BASE="$IDENT_ROOT"
if [[ $DEMO = 1 ]]; then SUFFIX="-Demo"; IDENT_BASE="$IDENT_ROOT.demo"; fi

if [[ $FROM_DIST = 1 ]]; then
  DSUF=""; DFLAG=""
  if [[ $DEMO = 1 ]]; then DSUF="-demo"; DFLAG=" --demo"; fi
  D="$REPO_ROOT/dist/desipper-v${VERSION}${DSUF}"
  [[ -d "$D" ]] || { echo "missing $D - run ./distribute.sh ${VERSION}${DFLAG} first"; exit 1; }
  VST3="$D/VST3/Sipper.vst3"; AU="$D/AU/Sipper.component"; AAX="$D/AAX/Sipper.aaxplugin"
  [[ -d "$D/Standalone/De-Sipper.app" ]] && APP="$D/Standalone/De-Sipper.app"
fi

# Branded wizard pages are required by the Shipping Playbook - fail without them.
for f in sidebar.png welcome.html license.html conclusion.html conclusion-demo.html; do
  [[ -f "$RESOURCES/$f" ]] || { echo "missing installer resource $RESOURCES/$f"; exit 1; }
done

WORK="$(mktemp -d)"
trap 'rc=$?; rm -rf "$WORK"; exit $rc' EXIT
PKGS=()

# Resolve @PRODUCT@ / @VERSION@ and pick the paid or demo conclusion page.
RES="$WORK/Resources"; mkdir -p "$RES"
cp "$RESOURCES/sidebar.png" "$RESOURCES/license.html" "$RES/"
CONC="conclusion.html"; [[ $DEMO = 1 ]] && CONC="conclusion-demo.html"
for pair in "welcome.html:welcome.html" "$CONC:conclusion.html"; do
  sed -e "s/@PRODUCT@/$PRODUCT/g" -e "s/@VERSION@/$VERSION/g" "$RESOURCES/${pair%%:*}" > "$RES/${pair##*:}"
done

component() { # $1 bundle path, $2 dest dir, $3 pkg suffix
  local src="$1" dest="$2" suf="$3"
  [[ -n "$src" ]] || { echo "  (skip $suf - not given)"; return 0; }
  [[ -d "$src" ]] || { echo "missing $suf bundle: $src"; exit 1; }
  local root="$WORK/root_$suf"
  mkdir -p "$root$dest"
  # ditto, never cp -R: cp corrupts the PACE symlink inside the AAX bundle.
  ditto "$src" "$root$dest/$(basename "$src")"
  pkgbuild --root "$root" --identifier "$IDENT_BASE.$suf" --version "$VERSION" \
           --install-location "/" "$WORK/$suf.pkg" >/dev/null
  PKGS+=("$suf")
  echo "  packaged $suf"
}

echo "==> $PRODUCT v$VERSION$SUFFIX"
echo "==> [1/5] Component packages"
component "$VST3" "/Library/Audio/Plug-Ins/VST3"                     vst3
component "$AU"   "/Library/Audio/Plug-Ins/Components"               au
component "$AAX"  "/Library/Application Support/Avid/Audio/Plug-Ins" aax
component "$APP"  "/Applications"                                    app

[[ ${#PKGS[@]} -gt 0 ]] || { echo "nothing to package"; exit 1; }

echo "==> [2/5] Distribution (hostArchitectures=arm64,x86_64)"
DIST="$WORK/distribution.xml"
{
  echo '<?xml version="1.0" encoding="utf-8"?>'
  echo '<installer-gui-script minSpecVersion="1">'
  echo "    <title>$PRODUCT${SUFFIX:+ Demo}</title>"
  echo '    <welcome    file="welcome.html"/>'
  echo '    <license    file="license.html"/>'
  echo '    <conclusion file="conclusion.html"/>'
  echo '    <background          file="sidebar.png" alignment="bottomleft" scaling="proportional"/>'
  echo '    <background-darkAqua file="sidebar.png" alignment="bottomleft" scaling="proportional"/>'
  echo '    <options customize="never" require-scripts="false" hostArchitectures="arm64,x86_64"/>'
  echo '    <choices-outline>'
  echo '        <line choice="default">'
  for n in "${PKGS[@]}"; do echo "            <line choice=\"$IDENT_BASE.$n\"/>"; done
  echo '        </line>'
  echo '    </choices-outline>'
  echo '    <choice id="default"/>'
  for n in "${PKGS[@]}"; do
    echo "    <choice id=\"$IDENT_BASE.$n\" visible=\"false\"><pkg-ref id=\"$IDENT_BASE.$n\"/></choice>"
    echo "    <pkg-ref id=\"$IDENT_BASE.$n\" version=\"$VERSION\" onConclusion=\"none\">$n.pkg</pkg-ref>"
  done
  echo '</installer-gui-script>'
} > "$DIST"

UNSIGNED="$WORK/$PRODUCT-v${VERSION}${SUFFIX}-unsigned.pkg"
FINAL="$REPO_ROOT/dist/$PRODUCT-v${VERSION}${SUFFIX}-Installer.pkg"
mkdir -p "$REPO_ROOT/dist"

productbuild --distribution "$DIST" --resources "$RES" \
             --package-path "$WORK" \
             "$UNSIGNED" >/dev/null

echo "==> [3/5] Release gate"
"$GATE" "$UNSIGNED"

echo "==> [4/5] productsign"
rm -f "$FINAL"
productsign --sign "$INSTALLER_ID" "$UNSIGNED" "$FINAL" >/dev/null

echo "==> [5/5] Notarize + staple"
xcrun notarytool submit "$FINAL" --keychain-profile "$NOTARY_PROFILE" --wait
xcrun stapler staple "$FINAL"
xcrun stapler validate "$FINAL"

"$GATE" "$FINAL"
spctl --assess --type install "$FINAL" && echo "  Gatekeeper: PASSED"

echo
echo "Done: $FINAL"
pkgutil --check-signature "$FINAL" | head -3
