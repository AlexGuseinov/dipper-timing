# Shared settings for the IoT-LAB scripts (sourced).
IOTLAB_LOGIN=${IOTLAB_LOGIN:-huseynli}
IOTLAB_KEY=${IOTLAB_KEY:-$HOME/.ssh/iotlab_rsa}
REPO=$(cd "$(dirname "$0")/../.." && pwd)
fe() { _fe_site=$1; shift; ssh -i "$IOTLAB_KEY" -o ServerAliveInterval=60 "$IOTLAB_LOGIN@$_fe_site.iot-lab.info" "$@"; }
