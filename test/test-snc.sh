#!/usr/bin/env bash
set -e

SCRIPT_DIR=$(dirname -- "$(readlink -f -- "${BASH_SOURCE[0]}")")
SNC="${SCRIPT_DIR}/../snc"

# void test_random(bytes);
test_random() {
    $SNC --receive > /dev/null &
    sleep 0.25

    tr -dc A-Za-z0-9 </dev/urandom | head -c "$1" | $SNC --transmit 'localhost'
    sleep 0.25

    echo "Successfully transmitted and received $1 bytes."
}

test_random 1
test_random 10
test_random 100
test_random 4096
test_random 5376
test_random 8192
