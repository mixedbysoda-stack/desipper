#!/bin/bash
# build_installer.sh — macOS PKG: VST3 + AU + signed AAX (+ optional Standalone)
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
# Usage:
#   ./scripts/build_installer.sh --version 1.0.1 \
#     --vst3 "build/Sipper_artefacts/Release/VST3/Sipper.vst3" \
#     --au   "build/Sipper_artefacts/Release/AU/Sipper.component" \
#     --aax  "dist/Sipper.aaxplugin" \
#     [--app "build/Sipper_artefacts/Release/Standalone/De-Sipper.app"]
#
# The AAX input must already be PACE-wrapped and signed - packaging does not
# invalidate the wrap, but rebuilding does.
set -euo pipefail

INSTALLER_ID="Developer ID Installer: Miguel Silverio (K7FRP77ZZK)"
NOTARY_PROFILE="carbonator-notary"
IDENT_BASE="com.carbonatedaudio.desipper"
REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
RESOURCES="$REPO_ROOT/installer/Resources"
GATE="$REPO_ROOT/scripts/check_pkg_arch.sh"

VERSION=""; VST3=""; AU=""; AAX=""; APP=""
while [[ $# -gt 0 ]]; do
  case "$1" in
    --version) VERSION="$2"; shift 2;;
    --vst3) VST3="$2"; shift 2;;
    --au)   AU="$2";   shift 2;;
    --aax)  AAX="$2";  shift 2;;
    --app)  APP="$2";  shift 2;;
    *) echo "unknown arg: $1"; exit 1;;
  esac
done
[[ -n "$VERSION" ]] || { echo "--version is required"; exit 1; }

# Branded wizard pages are required by the Shipping Playbook. Build without
# them rather than fail, but say so loudly - it is not release-ready.
HAVE_RESOURCES=1
for f in sidebar.png welcome.html license.html conclusion.html; do
  [[ -f "$RESOURCES/$f" ]] || { echo "WARNING: missing installer resource $RESOURCES/$f"; HAVE_RESOURCES=0; }
done
[[ $HAVE_RESOURCES = 1 ]] || echo "WARNING: building a plain (unbranded) wizard - not release-ready per Playbook."

WORK="$(mktemp -d)"
trap 'rc=$?; rm -rf "$WORK"; exit $rc' EXIT
PKGS=()

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
  echo "    <title>De-Sipper</title>"
  if [[ $HAVE_RESOURCES = 1 ]]; then
    echo '    <welcome    file="welcome.html"/>'
    echo '    <license    file="license.html"/>'
    echo '    <conclusion file="conclusion.html"/>'
    echo '    <background          file="sidebar.png" alignment="bottomleft" scaling="proportional"/>'
    echo '    <background-darkAqua file="sidebar.png" alignment="bottomleft" scaling="proportional"/>'
  fi
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

UNSIGNED="$WORK/De-Sipper-${VERSION}-unsigned.pkg"
FINAL="$REPO_ROOT/dist/De-Sipper-v${VERSION}-Installer.pkg"
mkdir -p "$REPO_ROOT/dist"

RES_ARGS=()
[[ $HAVE_RESOURCES = 1 ]] && RES_ARGS=(--resources "$RESOURCES")
productbuild --distribution "$DIST" ${RES_ARGS[@]+"${RES_ARGS[@]}"} \
             --package-path "$WORK" \
             "$UNSIGNED" >/dev/null

echo "==> [3/5] Release gate"
"$GATE" "$UNSIGNED"

echo "==> [4/5] productsign"
productsign --sign "$INSTALLER_ID" "$UNSIGNED" "$FINAL" >/dev/null

echo "==> [5/5] Notarize + staple"
xcrun notarytool submit "$FINAL" --keychain-profile "$NOTARY_PROFILE" --wait
xcrun stapler staple "$FINAL"
xcrun stapler validate "$FINAL"

"$GATE" "$FINAL"

echo
echo "Done: $FINAL"
pkgutil --check-signature "$FINAL" | head -3
