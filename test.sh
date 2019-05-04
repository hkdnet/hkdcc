#!/bin/bash

# for debug, you can try `$ bash -x test.sh`
try() {
  expected="$1"
  input="$2"

  build/hkdcc "$input" > build/tmp.s
  gcc -o build/tmp dummy.o build/tmp.s
  build/tmp
  actual="$?"

  if [ "$actual" = "$expected" ]; then
    echo "$input => $actual"
  else
    echo "$input => NG: $expected expected, but got $actual"
    exit 1
  fi
}

# ptr
try 3 '
int main() {
  int x;
  x = 3;
  int *y;
  y = &x;
  return *y;
}
'

echo OK

