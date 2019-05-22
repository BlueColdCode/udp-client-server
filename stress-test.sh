#!/bin/bash

for k in {1..100000}; do
    for i in {1..4}; do
        ./client localhost 9090 $k$i &
        #echo $k$i
    done
    sleep 0.001
done

