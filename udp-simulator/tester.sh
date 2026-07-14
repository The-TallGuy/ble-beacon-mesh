#!/bin/bash

# Exit immediately if a command exits with a non-zero status
set -e 

echo "[*] Compiling 'victim' ..."
# Linking OpenSSL for the HMAC function
g++ victim.cpp public/MurmurHash3.cpp -o victim -lcrypto -Wall
echo "[+] Compilation successful."
echo "-----------------------------------"

# Execution Logic based on input parameter
if [ -z "$1" ]; then
    echo "Usage: ./tester.sh [scenario_number]"
    echo "  0 : Compile only"
    echo "  1 : Run Listener"
    echo "  2 : Run Broadcaster (No GPS - Subway)"
    echo "  3 : Run Broadcaster (With GPS - Hiker)"
    exit 1
fi

case $1 in
    0)
        echo "[*] Ready."
        ;;
    1)
        echo "[*] Starting Listener..."
        ./listener
        ;;
    2)
        echo "[*] Starting Broadcaster: Subway Scenario (No GPS)"
        ./victim "+40711111111" 0
        ;;
    3)
        echo "[*] Starting Broadcaster: Hiker Scenario (GPS Valid)"
        ./victim "+40722222222" 1 44.4398 26.0428
        ;;
    *)
        echo "[!] Invalid scenario number."
        exit 1
        ;;
esac