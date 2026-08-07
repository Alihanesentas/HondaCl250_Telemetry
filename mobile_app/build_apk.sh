#!/bin/bash
# Honda CL250 Telemetry - Native Android APK Build Script

echo "=========================================================="
echo "   HONDA CL250 NATIVE ANDROID APK BUILD PROCESS           "
echo "=========================================================="

# Environment paths for Flutter and Java 17
if [ -d "$HOME/development/flutter/bin" ]; then
    export PATH="$HOME/development/flutter/bin:$PATH"
fi

if [ -d "/opt/homebrew/opt/openjdk@17" ]; then
    export JAVA_HOME="/opt/homebrew/opt/openjdk@17"
    export PATH="$JAVA_HOME/bin:$PATH"
fi

CD_DIR="$(dirname "$0")/flutter_app"
cd "$CD_DIR" || exit 1

echo "1. Fetching Flutter dependencies..."
flutter pub get

echo "2. Building release Android APK package..."
flutter build apk --release --android-skip-build-dependency-validation

mkdir -p ../release_apk
cp build/app/outputs/flutter-apk/app-release.apk ../release_apk/HondaCL250_Telemetry.apk
cp build/app/outputs/flutter-apk/app-release.apk ../HondaCL250_Telemetry.apk

echo ""
echo "=========================================================="
echo "   APK BUILD COMPLETE!                                    "
echo "=========================================================="
echo "Release APK copies generated:"
echo " 1) mobile_app/release_apk/HondaCL250_Telemetry.apk"
echo " 2) mobile_app/HondaCL250_Telemetry.apk"
echo " 3) mobile_app/flutter_app/build/app/outputs/flutter-apk/app-release.apk"
echo "Install on your Android phone via USB ADB or direct download."
echo "=========================================================="
