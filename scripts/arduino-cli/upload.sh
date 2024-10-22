#!/bin/bash

set -euo pipefail


. ./scripts/arduino-cli/default-port.sh

arduino-cli upload --fqbn arduino:renesas_uno:unor4wifi -i .arduino-cli-build/blastic-scale.ino.bin -p "$BLASTIC_SCALE_PORT" "${@}"
