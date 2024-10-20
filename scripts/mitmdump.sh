#!/bin/bash

set -euo pipefail

sudo nft destroy table mitmproxy
sudo nft add table mitmproxy
trap 'sudo nft destroy table mitmproxy' EXIT
sudo nft add chain mitmproxy intercept \{ type nat hook prerouting priority dstnat\; \}
sudo nft add rule mitmproxy intercept tcp dport \{ 80, 443 \} redirect to :8080

SSLKEYLOGFILE=mitmdump-keylog.txt mitmdump --mode transparent "${@}"
