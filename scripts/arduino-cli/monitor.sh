#!/bin/bash

set -euo pipefail

. ./scripts/arduino-cli/default-port.sh

arduino-cli monitor --fqbn arduino:renesas_uno:unor4wifi -p "$BLASTIC_SCALE_PORT" --config baudrate=115200 "${@}"
