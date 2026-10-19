#!/bin/sh
# Copy common/, impl/, firmware/riot/ and scripts/ to ~/dipper-timing on a frontend.
#   scripts/iotlab/sync.sh [site]        (default grenoble, where RIOT is built)
set -eu
. "$(dirname "$0")/common.sh"
site=${1:-grenoble}   # RIOT builds happen on grenoble; other sites need scripts/ only
cd "$REPO"
COPYFILE_DISABLE=1 tar --no-xattrs -czf - common impl firmware/riot/Makefile firmware/riot/main.c \
    firmware/riot/platform.h firmware/riot/dipper_mod scripts \
  | fe "$site" 'mkdir -p dipper-timing && cd dipper-timing && tar xzmf -'
echo "synced to $site"
