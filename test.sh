#!/bin/bash

for (( i=1; i<=19; i++))
do
  ./parser "tests/"$i".cmm"
done
