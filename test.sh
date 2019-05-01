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

# single value
try 0 'main() { 0; }'
try 42 'main() { 42; }'

# add
try 3 'main() { 1+2; }'
try 13 'main() { 1+10+2; }'

# sub
try 2 'main() { 3-1; }'
try 11 'main() { 23-12; }'

# space
try 2 'main() { 3 - 1; }'
# multi
try 120 'main() { 70+2*5*5; }'
# div
try 2 'main() { 4/2; }'
try 47 'main() { 5+6*7; }'
try 15 'main() { 5*(9-6); }'
try 4 'main() { (3+5)/2; }'

# multi statements
try 2 'main() { 1; 2; }'
# var
try 1 'main() { a = 1; }'
try 1 'main() { a = 1; a; }'
try 2 'main() { a = 1; a + 1; }'
try 2 'main() { a = b = 1; a + b; }'

# var of 2 or more characters
try 1 'main() { ab = 1; }'
try 1 'main() { ab = 1; ab; }'
try 2 'main() { ab = 1; ab + 1; }'
try 2 'main() { ab = cd = 1; ab + cd; }'
try 3 'main() { a = 1; ab = 2; a + ab; }'

# eqeq
try 1 'main() { 1 == 1; }'
try 1 'main() { a = 1; 1 == a; }'
try 0 'main() { a = 2; 1 == a; }'
try 0 'main() { a = 2; a == 1; }'
try 0 'main() { a = 0 == 1; a; }'


# noteq
try 0 'main() { 1 != 1; }'
try 1 'main() { 2 != 1; }'

# call
try 1 'main() { foo(); }'
try 3 'main() { bar(1, 2); }'

# funcdef
try 3 "
my_func() { 3; }
main() { my_func(); }
"

try 4 "
my_func2(a) { a + 1; }
main() { my_func2(3); }
"

echo OK

