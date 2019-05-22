#!/bin/bash

./client localhost 9090 123 &
./client localhost 9090 120 &
./client localhost 9090 543 &
./client localhost 9090 5 &

