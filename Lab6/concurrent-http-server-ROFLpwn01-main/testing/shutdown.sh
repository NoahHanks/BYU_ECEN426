#!/usr/bin/env bash

# Get pid of http server
PID=$(pgrep -u philipbl -f "python3 http_server.py")

if [ -z "$PID" ]; then
      echo "Server is not started."
      exit
fi

curl -v -sS http://localhost:8085/page.html 2>&1 &
sleep .1
curl -v -sS http://localhost:8085/page.html 2>&1 &
sleep .1
curl -v -sS http://localhost:8085/page.html 2>&1 &

# Wait for a bit before killing server
sleep 1

# Send ctrl-c to http server
kill -SIGINT $PID

wait
