#!/bin/bash

emcc bouncing.c draw.c -o dist/bouncing.html --shell-file wasm_shell.html -sEXPORTED_RUNTIME_METHODS=ccall,cwrap -sEXPORTED_FUNCTIONS=_main,_NumString --extern-post-js main.js --emrun
