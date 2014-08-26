#!/bin/sh

ifconfig eth0 | grep 'inet addr' | awk 'BEGIN{FS=" +|:"} {print $4}' | tr -d '\n'
