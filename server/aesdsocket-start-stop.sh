#!/bin/sh
#
DAEMON_NAME="aesdsocket"
AESD_SOCKET_PATH="/usr/bin/"

# Kill the server if it was already running.

start-stop-daemon -K --signal TERM --name ${DAEMON_NAME}

# Start up the daemon process

start-stop-daemon -S --exec ${AESD_SOCKET_PATH}${DAEMON_NAME} -- -d
