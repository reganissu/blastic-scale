#!/bin/bash

set -euo pipefail

if [[ $# -ne 1 ]]; then
  echo "Usage: $0 <path to uno-r4-wifi-usb-bridge firmware file>"
fi
firmware="$(realpath "$1")"

cd "$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
cp -v "$firmware" "$firmware".orig

cat deps/uno-r4-wifi-usb-bridge/certificates/cacrt_all.pem ~/.mitmproxy/mitmproxy-ca-cert.cer > uno-r4-wifi-usb-bridge-trust-with-mitmproxy.pem
python deps/uno-r4-wifi-usb-bridge/certificates/gen_crt_bundle.py -i uno-r4-wifi-usb-bridge-trust-with-mitmproxy.pem
mv x509_crt_bundle uno-r4-wifi-usb-bridge-trust-with-mitmproxy.esp-idf-trust
if [[ $(stat --printf="%s" uno-r4-wifi-usb-bridge-trust-with-mitmproxy.esp-idf-trust) -gt 282624 ]]; then
  echo 'uno-r4-wifi-usb-bridge-trust-with-mitmproxy.esp-idf-trust is larger than certs map size (0x045000)!' 1>&2
  exit 1;
fi
dd if=uno-r4-wifi-usb-bridge-trust-with-mitmproxy.esp-idf-trust of="$firmware" bs=4096 seek=11 conv=notrunc

echo "mitmproxy ca has been added to the trust anchor, now flash $firmware on your board"
