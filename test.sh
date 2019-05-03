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
try 0 'int main() { return 0; }'

try 42 'int main() { return 42; }'

# unary operator
try 1 'int main() { return +1; }'
try 255 'int main() { return -1; }'

# add
try 3 'int main() { return 1+2; }'
try 13 'int main() { return 1+10+2; }'

# sub
try 2 'int main() { return 3-1; }'
try 11 'int main() { return 23-12; }'

# space
try 2 'int main() { return 3 - 1; }'
# multi
try 120 'int main() { return 70+2*5*5; }'
# div
try 2 'int main() { return 4/2; }'
try 47 'int main() { return 5+6*7; }'
try 15 'int main() { return 5*(9-6); }'
try 4 'int main() { return (3+5)/2; }'

# multi statements
try 2 'int main() { 1; return 2; }'
# var
try 1 'int main() { int a; return a = 1; }'
try 1 'int main() { int a; a = 1; return a; }'
try 2 'int main() { int a; a = 1; return a + 1; }'
try 2 'int main() { int a; int b; a = b = 1; return a + b; }'

# var of 2 or more characters
try 1 'int main() { int ab; return ab = 1; }'
try 1 'int main() { int ab; ab = 1; return ab; }'
try 2 'int main() { int ab; ab = 1; return ab + 1; }'
try 2 'int main() { int ab; int cd; ab = cd = 1; return ab + cd; }'
try 3 'int main() { int a; int ab; a = 1; ab = 2; return a + ab; }'

# eqeq
try 1 'int main() { return 1 == 1; }'
try 1 'int main() { int a; a = 1; return 1 == a; }'
try 0 'int main() { int a; a = 2; return 1 == a; }'
try 0 'int main() { int a; a = 2; return a == 1; }'
try 0 'int main() { int a; a = 0 == 1; return a; }'

# not eq
try 0 'int main() { return 1 != 1; }'
try 1 'int main() { return 2 != 1; }'

# <=, <, >=, >
try 0 'int main() { return 1 <= 0; }'

# if
try 1 'int main() { if (1==1) return 1; return 0; }'
try 0 'int main() { if (1==0) return 1; return 0; }'
try 0 'int main() { if (1!=1) return 1; return 0; }'
try 1 'int main() { if (1!=0) return 1; return 0; }'
try 1 'int main() { int a; if (a=1) return 1; return 0; }'
try 0 'int main() { int a; if (a=0) return 1; return 0; }'
try 1 'int main() { int a; if (a=1) return a; return 2; }'
try 0 'int main() { int a; if (a=0) return 2; return a; }'
try 0 'int main() { if (1) return 0; return 1; }'
try 1 'int main() { if (0) return 0; return 1; }'
try 0 'int main() { if (1) if(1) return 0; return 1; }'
try 1 'int main() { if (1) if(0) return 0; return 1; }'
try 0 'int main() { int a; a = 1; if (1) a = 0; return a; }'
try 1 'int main() { int a; a = 1; if (0) a = 0; return a; }'

# while
try 0 'int main() { int a; a = 3; while (a != 0) a = baz(a); return a; }'
try 0 'int main() { int a; a = 1; while (a = 0) return 128; return a; }'

# call
try 1 'int main() { return foo(); }'
try 3 'int main() { return bar(1, 2); }'

# funcdef
try 3 "
int my_func() { return 3; }
int main() { return my_func(); }
"

try 4 "
int my_func2(a) { return a + 1; }
int main() { return my_func2(3); }
"

echo OK

