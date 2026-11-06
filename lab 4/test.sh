#!/usr/bin/env bash

for i in {1..10}; 
do
    curl -s "http://localhost:8080/square?num=$i"
done

wait
echo