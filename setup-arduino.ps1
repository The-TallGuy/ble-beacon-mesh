param (
    [ValidateSet("relay", "victim")]
    [string]$Target = "relay"
)

$SketchName = "ArduinoBuild"
$SourceDir = ".\arduino"

# Clean old build
if (Test-Path $SketchName) {
    Remove-Item -Path $SketchName -Recurse -Force
}
New-Item -ItemType Directory -Path $SketchName | Out-Null

# Copy source headers and cpp files
Copy-Item -Path "$SourceDir\include\*" -Destination $SketchName -Force
Copy-Item -Path "$SourceDir\src\*" -Destination $SketchName -Force

# We need MurmurHash3 from public/ if it's not already in src/
# Assuming it was moved to arduino/src/ based on our earlier structure

# Format for Arduino IDE (Sketch name must match folder name)
if ($Target -eq "relay") {
    Rename-Item -Path "$SketchName\esp-relay.cpp" -NewName "$SketchName.ino"
    Remove-Item -Path "$SketchName\esp-victim.cpp" -Force
} else {
    Rename-Item -Path "$SketchName\esp-victim.cpp" -NewName "$SketchName.ino"
    Remove-Item -Path "$SketchName\esp-relay.cpp" -Force
}

# Remove stray UDP files if they accidentally copied over
Remove-Item -Path "$SketchName\node.cpp" -ErrorAction SilentlyContinue
Remove-Item -Path "$SketchName\victim.cpp" -ErrorAction SilentlyContinue

Write-Host "Success: $SketchName created for target '$Target'." -ForegroundColor Green