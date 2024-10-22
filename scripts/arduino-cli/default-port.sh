#!/bin/bash

if [[ -z "$BLASTIC_SCALE_PORT" ]]; then
  BLASTIC_SCALE_PORT="$(arduino-cli board list --fqbn arduino:renesas_uno:unor4wifi --json | \
    jq -r -e '.detected_ports[].port.address, halt_error(if .detected_ports | length != 1 then 10 else 0 end)' 2>/dev/null)"
  if [[ $? != 0 ]]; then
    unset BLASTIC_SCALE_PORT
    echo cannot detect a single arduino:renesas_uno:unor4wifi board, define BLASTIC_SCALE_PORT manually 1>&2
    exit 1
  fi
fi
echo "$BLASTIC_SCALE_PORT"
export BLASTIC_SCALE_PORT
