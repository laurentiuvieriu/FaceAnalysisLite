#!/bin/bash

echo "Firing the launch server in a new terminal ..."

gnome-terminal -x /bin/bash -c "cd ./build/exe/launchServer/; ./launchServer ../../../exe/launchServer/settings.ini"

echo "done! Now launching the launch client in a second terminal ..."

gnome-terminal -e ./build/exe/launchClient/launchClient&

echo "done! Now launching the USM process (in a third terminal) ..."

gnome-terminal -x /bin/bash -c "./build/exe/usmSub/usmSub -pubPort tcp://*:6000 -subPort tcp://*:6001 -interval 5"

echo "done"&
