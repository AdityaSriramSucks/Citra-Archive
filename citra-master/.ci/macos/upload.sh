#!/bin/bash -ex

. .ci/common/pre-upload.sh

REV_NAME="citra-osx-${GITDATE}-${GITREV}"
ARCHIVE_NAME="${REV_NAME}.tar.gz"
COMPRESSION_FLAGS="-czvf"

mkdir "$REV_NAME"

cp build/bin/Release/citra "$REV_NAME"
cp -r build/bin/Release/citra-qt.app "$REV_NAME"
cp build/bin/Release/citra-room "$REV_NAME"

# move libs into folder for deployment
macpack "${REV_NAME}/citra-qt.app/Contents/MacOS/citra-qt" -d "../Frameworks"
# move qt frameworks into app bundle for deployment
macdeployqt "${REV_NAME}/citra-qt.app" -executable="${REV_NAME}/citra-qt.app/Contents/MacOS/citra-qt"

# move libs into folder for deployment
macpack "${REV_NAME}/citra" -d "libs"

# workaround for libc++
install_name_tool -change @loader_path/../Frameworks/libc++.1.0.dylib /usr/lib/libc++.1.dylib "${REV_NAME}/citra-qt.app/Contents/MacOS/citra-qt"
install_name_tool -change @loader_path/libs/libc++.1.0.dylib /usr/lib/libc++.1.dylib "${REV_NAME}/citra"

# Make the launching script executable
chmod +x ${REV_NAME}/citra-qt.app/Contents/MacOS/citra-qt

# Verify loader instructions
find "$REV_NAME" -type f -exec otool -L {} \;

. .ci/common/post-upload.sh
