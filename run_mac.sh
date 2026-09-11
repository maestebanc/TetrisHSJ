#!/usr/bin/env bash
set -e
cd "$(dirname "$0")"
if [ -d "SantJoanTetris.app" ]; then
    open SantJoanTetris.app || ./SantJoanTetris.app/Contents/MacOS/SantJoanTetris "$@"
elif [ -f "./sant_joan_tetris_mac" ]; then
    ./sant_joan_tetris_mac "$@"
fi
