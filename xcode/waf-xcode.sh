#!/bin/bash
# Expecting PebbleSDK + arm toolchain + openocd binaries to be in $PATH after sourcing .bash_profile:
source ~/.bash_profile
cd ..
if [ -z $ACTION ]; then
    ACTION=build
fi
# Check if waf is on the path:
if ! type "waf" &> /dev/null; then
    ./waf $ACTION
else
    waf $ACTION
fi
