#!/bin/bash

echo "Firing the launch server in a new terminal ..."

gnome-terminal -x /bin/bash -c "cd ./build/exe/launchServer/; ./launchServer ../../../exe/launchServer/settings.ini"

echo "done! Now launching the launch client in a second terminal ..."

gnome-terminal -e ./build/exe/launchClient/launchClient&

echo "done! Now launching a subscriber to the face analysis component (in a third terminal) ..."

gnome-terminal -e ./build/exe/faceSub/faceSub&

echo "done"&
