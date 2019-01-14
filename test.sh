#!/bin/bash

# for debug, you can try `$ bash -x test.sh`
try() {
  expected="$1"
  input="$2"

  build/hkdcc "$input" > build/tmp.s
  gcc -o build/tmp build/tmp.s
  build/tmp
  actual="$?"

  if [ "$actual" = "$expected" ]; then
    echo "$input => $actual"
  else
    echo "$input => NG: $expected expected, but got $actual"
    exit 1
  fi
}

# single value
try 0 '0;'
try 42 '42;'

# add
try 3 '1+2;'
try 13 '1+10+2;'

# sub
try 2 '3-1;'
try 11 '23-12;'

# space
try 2 '3 - 1;'

# multi
try 120 '70+2*5*5;'
# div
try 2 '4/2;'

try 47 "5+6*7;"
try 15 "5*(9-6);"
try 4 "(3+5)/2;"

# multi statements
try 2 "1; 2;"

# var
try 1 "a = 1;"
try 1 "a = 1; a;"
try 2 "a = 1; a + 1;"
try 2 "a = b = 1; a + b;"

# var of 2 or more characters
try 1 "ab = 1;"
try 1 "ab = 1; ab;"
try 2 "ab = 1; ab + 1;"
try 2 "ab = cd = 1; ab + cd;"
try 3 "a = 1; ab = 2; a + ab;"

# eqeq
try 1 "1 == 1;"
try 1 "a = 1; 1 == a;"
try 0 "a = 2; 1 == a;"
try 0 "a = 2; a == 1;"
try 1 "a = 2; a == a;"
try 1 "a = 1 == 1; a;"
try 0 "a = 0 == 1; a;"

# noteq
try 0 "1 != 1;"
try 1 "2 != 1;"

echo OK
