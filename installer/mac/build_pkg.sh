#!/usr/bin/env bash
# =============================================================================
#  Crea l'installer macOS (.pkg) di Wanted Level
#
#  Uso:   installer/mac/build_pkg.sh <cartella_artefatti_Release> <versione>
#  Es.:   installer/mac/build_pkg.sh build/WantedLevel_artefacts/Release 1.0.0
#
#  Facoltativo (firma con certificato Apple Developer):
#    APP_SIGN_ID="Developer ID Application: Nome (TEAMID)"
#    PKG_SIGN_ID="Developer ID Installer: Nome (TEAMID)"
# =============================================================================
set -euo pipefail

ART="${1:?cartella artefatti mancante}"
VERSION="${2:-1.0.0}"
HERE="$(cd "$(dirname "$0")" && pwd)"
WORK="$(pwd)/pkgwork"
OUT="$(pwd)/dist"
NAME="Wanted Level"
ID="com.tunetalks.wantedlevel"

rm -rf "$WORK" && mkdir -p "$WORK"/{vst3,au,app,pkgs} "$OUT"

cp -R "$ART/VST3/$NAME.vst3"           "$WORK/vst3/"
cp -R "$ART/AU/$NAME.component"        "$WORK/au/"
cp -R "$ART/Standalone/$NAME.app"      "$WORK/app/"

# --- firma dei bundle (ad-hoc se non c'e' un certificato) ---------------------
SIGN="${APP_SIGN_ID:--}"
for b in "$WORK/vst3/$NAME.vst3" "$WORK/au/$NAME.component" "$WORK/app/$NAME.app"; do
    if [ "$SIGN" = "-" ]; then
        codesign --force --deep -s - "$b"
    else
        codesign --force --deep --options runtime --timestamp -s "$SIGN" "$b"
    fi
done

# --- un pacchetto per componente --------------------------------------------
make_component () {   # $1 = root, $2 = id, $3 = install location, $4 = nome file
    local plist="$WORK/$2.plist"
    pkgbuild --analyze --root "$1" "$plist"
    # i plug-in vanno sempre nella cartella di sistema, mai "spostati" altrove
    /usr/libexec/PlistBuddy -c "Set :0:BundleIsRelocatable false" "$plist"
    pkgbuild --root "$1" --component-plist "$plist" \
             --identifier "$2" --version "$VERSION" \
             --install-location "$3" "$WORK/pkgs/$4"
}

make_component "$WORK/vst3" "$ID.vst3"       "/Library/Audio/Plug-Ins/VST3"       vst3.pkg
make_component "$WORK/au"   "$ID.au"         "/Library/Audio/Plug-Ins/Components" au.pkg
make_component "$WORK/app"  "$ID.standalone" "/Applications"                      app.pkg

# --- pacchetto finale con scelta dei componenti ------------------------------
sed "s/@VERSION@/$VERSION/g" "$HERE/distribution.xml" > "$WORK/distribution.xml"

PKG="$OUT/WantedLevel-$VERSION-macOS.pkg"
if [ -n "${PKG_SIGN_ID:-}" ]; then
    productbuild --distribution "$WORK/distribution.xml" --package-path "$WORK/pkgs" \
                 --resources "$HERE/resources" --sign "$PKG_SIGN_ID" "$PKG"
else
    productbuild --distribution "$WORK/distribution.xml" --package-path "$WORK/pkgs" \
                 --resources "$HERE/resources" "$PKG"
fi

echo "Creato: $PKG"
