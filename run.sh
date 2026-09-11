#!/bin/bash
# Script de ejecución para Sant Joan Tetris
DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$DIR"
exec ./sant_joan_tetris "$@"
