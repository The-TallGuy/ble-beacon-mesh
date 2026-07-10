#!/bin/bash

TARGET=${1:-relay}
SKETCH_NAME="ArduinoBuild"
SOURCE_DIR="./arduino"

if [[ "$TARGET" != "relay" && "$TARGET" != "victim" ]]; then
    echo "Error: Target must be 'relay' or 'victim'."
    exit 1
fi

rm -rf "$SKETCH_NAME"
mkdir -p "$SKETCH_NAME"

cp "$SOURCE_DIR"/include/* "$SKETCH_NAME"/ 2>/dev/null || true
cp "$SOURCE_DIR"/src/* "$SKETCH_NAME"/ 2>/dev/null || true

if [ "$TARGET" == "relay" ]; then
    mv "$SKETCH_NAME/esp-relay.cpp" "$SKETCH_NAME/$SKETCH_NAME.ino"
    rm -f "$SKETCH_NAME/esp-victim.cpp"
else
    mv "$SKETCH_NAME/esp-victim.cpp" "$SKETCH_NAME/$SKETCH_NAME.ino"
    rm -f "$SKETCH_NAME/esp-relay.cpp"
fi

rm -f "$SKETCH_NAME/node.cpp" "$SKETCH_NAME/victim.cpp"

echo -e "\e[32mSuccess: $SKETCH_NAME created for target '$TARGET'.\e[0m"