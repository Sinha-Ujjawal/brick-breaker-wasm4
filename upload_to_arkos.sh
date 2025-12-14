#!/bin/bash

set -xe

make
rsync -avz ./build/ "ark@$ARKOS_IP_ADDR:/roms/wasm4/brick-breaker-wasm4/"
