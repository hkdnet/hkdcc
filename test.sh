#!/bin/bash

# for debug, you can try `$ bash -x test.sh` 
try() {
  expected="$1"
  input="$2"

  bin/hkdcc "$input" > tmp.s
  gcc -o bin/tmp tmp.s
  bin/tmp
  actual="$?"

  if [ "$actual" = "$expected" ]; then
    echo "$input => $actual"
  else
    echo "$expected expected, but got $actual"
    exit 1
  fi
}

# single value
try 0 0
try 42 42

# add
try 3 1+2
try 13 1+10+2

# sub
try 2 3-1
try 11 23-12

# space
try 2 '3 - 1'

echo OK
