#!/usr/bin/env bash

curl -v -sS http://localhost:8085/page.html 2>&1 &
sleep .1
curl -v -sS http://localhost:8085/page.html 2>&1 &

wait
