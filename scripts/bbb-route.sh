#!/usr/bin/env sh
set -eu

TARGET_IP="10.129.1.152"
HOST_IF="eno0"
HOST_SRC="10.129.1.110"
VPN_TABLE="7113"

usage() {
    cat <<EOF
Usage: $0 check|apply|delete

check   Show the route Linux currently selects for the BeagleBone Black.
apply   Add or replace the temporary VPN bypass route.
delete  Remove the temporary route from the VPN policy table.

Target: $TARGET_IP
Route:  $TARGET_IP/32 dev $HOST_IF src $HOST_SRC table $VPN_TABLE
EOF
}

case "${1:-}" in
    check)
        ip route get "$TARGET_IP"
        ;;
    apply)
        sudo ip route replace "$TARGET_IP/32" dev "$HOST_IF" src "$HOST_SRC" table "$VPN_TABLE"
        ip route get "$TARGET_IP"
        ;;
    delete)
        sudo ip route del "$TARGET_IP/32" table "$VPN_TABLE"
        ip route get "$TARGET_IP"
        ;;
    *)
        usage
        exit 2
        ;;
esac
