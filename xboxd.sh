#!/bin/bash

function detect() {
  lsusb | grep 0f0d:000d > /dev/null
  NOT_FOUND=$?
}

function xbox_up() {
  /usr/bin/xboxdrv --daemon --detach --dbus disabled 2> /dev/null > /dev/null
}

function xbox_down() {
  pkill xboxdrv
}

PREV=""

while true; do
  detect
  if [ "$PREV" != "$NOT_FOUND" ]; then
    if [ "$NOT_FOUND" == "1" ]; then
      xbox_down
    else
      xbox_up
    fi
  fi
  PREV=$NOT_FOUND
  sleep 1
done
