#!/bin/bash

# Exit immediately if a command exits with a non-zero status
set -e 

echo "[*] Compiling 'victim' ..."
# Linking OpenSSL for the HMAC function
g++ node.cpp lib/circularBuff.cpp -o node -Wall
g++ victim.cpp public/MurmurHash3.cpp -o victim -lcrypto -Wall
echo "[+] Compilation successful."
echo "-----------------------------------"

# Execution Logic based on input parameter
if [ -z "$1" ]; then
    echo "Usage: ./tester.sh [scenario_number]"
    echo "  1 : Run Node"
    echo "  2 : Run Victim (No GPS - Subway)"
    echo "  3 : Run Victim (With GPS - Hiker)"
    exit 1
fi

case $1 in
    1)
        echo "[*] Starting Node..."
        ./node
        ;;
    2)
        echo "[*] Starting Victim: Subway Scenario (No GPS)"
        ./victim "+40711111111" 0
        ;;
    3)
        echo "[*] Starting Victim: Hiker Scenario (GPS Valid)"
        ./victim "+40722222222" 1 44.4398 26.0428
        ;;
    *)
        echo "[!] Invalid scenario number."
        exit 1
        ;;
esac