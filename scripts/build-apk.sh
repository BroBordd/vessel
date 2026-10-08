#!/bin/bash
set -e
cd "$(dirname "$0")/.."

BURN=0; [ "$1" = "--burn" ] && BURN=1

export ANDROID_HOME=$HOME/android-sdk
SDKM=$ANDROID_HOME/cmdline-tools/latest/bin/sdkmanager
BTV=35.0.0
BT=$ANDROID_HOME/build-tools/$BTV
JAR=$ANDROID_HOME/platforms/android-28/android.jar

if [ ! -x "$SDKM" ]; then
  sudo apt-get update -qq || true
  sudo apt-get install -y -qq openjdk-17-jdk-headless unzip wget zip
  mkdir -p $ANDROID_HOME/cmdline-tools
  wget -q https://dl.google.com/android/repository/commandlinetools-linux-11076708_latest.zip -O /tmp/ct.zip
  unzip -q /tmp/ct.zip -d $ANDROID_HOME/cmdline-tools
  mv $ANDROID_HOME/cmdline-tools/cmdline-tools $ANDROID_HOME/cmdline-tools/latest
  yes | $SDKM --licenses >/dev/null || true
fi
if [ ! -d "$BT" ] || [ ! -f "$JAR" ]; then
  $SDKM "platforms;android-28" "build-tools;$BTV"
fi

rm -rf build && mkdir -p build/classes build/src
cp -r app/src/. build/src/

if [ $BURN = 1 ]; then
  [ -f dist/libgame.so ] || { echo "dist/libgame.so missing, run scripts/burn.sh in Termux first"; exit 1; }
  sed -i 's/BURNED_IN = false/BURNED_IN = true/' $(find build/src -name MainActivity.java)
fi

$BT/aapt2 link -o build/base.apk -I $JAR --manifest app/AndroidManifest.xml

javac --release 8 -Xlint:-options -g:none -cp $JAR -d build/classes \
  $(find build/src -name '*.java')

$BT/d8 --lib $JAR --min-api 21 --output build \
  $(find build/classes -name '*.class')

(cd build && zip -q base.apk classes.dex)

if [ $BURN = 1 ]; then
  mkdir -p build/lib/arm64-v8a && cp dist/libgame.so build/lib/arm64-v8a/
  (cd build && zip -qr base.apk lib)
fi

$BT/zipalign -f -p 4 build/base.apk build/aligned.apk

[ -f debug.jks ] || keytool -genkey -keystore debug.jks -alias k \
  -keyalg RSA -keysize 2048 -validity 10000 \
  -storepass android -keypass android -dname "CN=dev"

$BT/apksigner sign --v4-signing-enabled false --ks debug.jks --ks-pass pass:android \
  --out vessel.apk build/aligned.apk

echo "built: $(pwd)/vessel.apk (burn=$BURN)"
