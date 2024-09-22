#!/bin/bash

set -euo pipefail

(
  cd openssl-keylog
  make
)

IPS=$(ip -4 -j addr show  | jq -r '[.[]?.addr_info[].local | select(test("^127") | not) | ("IP:" + ., "DNS:" + . + ".nip.io")] | sort | join(",")')

openssl genpkey -quiet -algorithm RSA -out socat-mitmproxy.key -pkeyopt rsa_keygen_bits:2048
cat <<-EOF > socat-mitmproxy.conf
  [req]
  distinguished_name = req_distinguished_name
  req_extensions = v3_req
  prompt = no

  [req_distinguished_name]
  CN = socat-mitmproxy

  [v3_req]
  keyUsage = critical, digitalSignature, keyEncipherment
  extendedKeyUsage = serverAuth
  subjectAltName = $IPS
EOF
openssl req -new -key socat-mitmproxy.key -out socat-mitmproxy.csr -config socat-mitmproxy.conf
openssl x509 -req -in socat-mitmproxy.csr -CA ~/.mitmproxy/mitmproxy-ca-cert.cer -CAkey ~/.mitmproxy/mitmproxy-ca.pem \
  -sha256 -CAcreateserial -days 1000 -extfile socat-mitmproxy.conf -extensions v3_req -out socat-mitmproxy.cer
cat socat-mitmproxy.key socat-mitmproxy.cer > socat-mitmproxy.pem

cat <<EOF

Generated certificate for subjectAltName = $IPS
Execute this serial command on the device:

tls::ping <ip>.nip.io 8443 [data...]

Note that currently certificate validation is broken on the Arduino UNO R4 WiFi for an IP (non DNS) address!

EOF
trap 'cat socat-mitmproxy-keylog.txt.* >> socat-mitmproxy-keylog.txt' EXIT
LD_PRELOAD="$(realpath openssl-keylog/libsslkeylog.so)" SSLKEYLOGFILE=socat-mitmproxy-keylog.txt socat -d -d openssl-listen:8443,cert=socat-mitmproxy.pem,verify=0,fork -
