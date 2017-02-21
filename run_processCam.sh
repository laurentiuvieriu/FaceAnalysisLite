#!/bin/bash

echo "Launching the processCam in a separate terminal ..."

gnome-terminal -x /bin/bash -c "cd ./build/exe/processCam/; ./processCam ../../../exe/processCam/settings.ini"

echo "done"
