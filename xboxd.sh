#!/bin/bash

function detect() {
  lsusb | grep 0f0d:000d > /dev/null
  NOT_FOUND=$?
}

function xbox_up() {
  echo "xboxdrv up"
  /usr/bin/xboxdrv --daemon --detach --dbus disabled 2> /dev/null > /dev/null
}

function xbox_down() {
  echo "xboxdrv down"
  pkill xboxdrv
}

CURRENT_STATE=""
NOT_FOUND=""

while true; do
  # |detect| sometime report a wrong result. Take only results that are
  # reported twice in sequence to stabilize it.
  LAST_STATE=$NOT_FOUND
  detect
  if [ "$LAST_STATE" == "$NOT_FOUND" ]; then

    # If newly detected state is different from the driver up/down state,
    # run xbox_up/xbox_down for the new state.
    if [ "$CURRENT_STATE" != "$NOT_FOUND" ]; then
      if [ "$NOT_FOUND" == "1" ]; then
        xbox_down
      else
        xbox_up
      fi
    fi
    CURRENT_STATE=$NOT_FOUND
  fi

  # Polling device in every 1 sec.
  sleep 1
done
