#!/bin/bash
# Honda CL250 Telemetry - Native Android APK Build Script

echo "=========================================================="
echo "   HONDA CL250 NATIVE ANDROID APK BUILD PROCESS           "
echo "=========================================================="

CD_DIR="$(dirname "$0")/flutter_app"
cd "$CD_DIR" || exit 1

echo "1. Fetching Flutter dependencies..."
flutter pub get

echo "2. Building release Android APK package..."
flutter build apk --release

echo ""
echo "=========================================================="
echo "   APK BUILD COMPLETE!                                    "
echo "=========================================================="
echo "Location: mobile_app/flutter_app/build/app/outputs/flutter-apk/app-release.apk"
echo "Install on your Android phone via USB ADB or direct download."
echo "=========================================================="
