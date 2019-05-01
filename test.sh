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
try 0 'main() { return 0; }'
try 42 'main() { return 42; }'

# add
try 3 'main() { return 1+2; }'
try 13 'main() { return 1+10+2; }'

# sub
try 2 'main() { return 3-1; }'
try 11 'main() { return 23-12; }'

# space
try 2 'main() { return 3 - 1; }'
# multi
try 120 'main() { return 70+2*5*5; }'
# div
try 2 'main() { return 4/2; }'
try 47 'main() { return 5+6*7; }'
try 15 'main() { return 5*(9-6); }'
try 4 'main() { return (3+5)/2; }'

# multi statements
try 2 'main() { 1; return 2; }'
# var
try 1 'main() { return a = 1; }'
try 1 'main() { a = 1; return a; }'
try 2 'main() { a = 1; return a + 1; }'
try 2 'main() { a = b = 1; return a + b; }'

# var of 2 or more characters
try 1 'main() { return ab = 1; }'
try 1 'main() { ab = 1; return ab; }'
try 2 'main() { ab = 1; return ab + 1; }'
try 2 'main() { ab = cd = 1; return ab + cd; }'
try 3 'main() { a = 1; ab = 2; return a + ab; }'

# eqeq
try 1 'main() { return 1 == 1; }'
try 1 'main() { a = 1; return 1 == a; }'
try 0 'main() { a = 2; return 1 == a; }'
try 0 'main() { a = 2; return a == 1; }'
try 0 'main() { a = 0 == 1; return a; }'


# noteq
try 0 'main() { return 1 != 1; }'
try 1 'main() { return 2 != 1; }'

# if
try 1 'main() { if (1==1) return 1; return 0; }'
try 0 'main() { if (1==0) return 1; return 0; }'
try 0 'main() { if (1!=1) return 1; return 0; }'
try 1 'main() { if (1!=0) return 1; return 0; }'
try 1 'main() { if (a=1) return 1; return 0; }'
try 0 'main() { if (a=0) return 1; return 0; }'
try 1 'main() { if (a=1) return a; return 2; }'
try 0 'main() { if (a=0) return 2; return a; }'
try 0 'main() { if (1) return 0; return 1; }'
try 1 'main() { if (0) return 0; return 1; }'
try 0 'main() { if (1) if(1) return 0; return 1; }'
try 1 'main() { if (1) if(0) return 0; return 1; }'

# while
try 0 'main() { a = 3; while (a != 0) a = baz(a); return a; }'

# call
try 1 'main() { return foo(); }'
try 3 'main() { return bar(1, 2); }'

# funcdef
try 3 "
my_func() { return 3; }
main() { return my_func(); }
"

try 4 "
my_func2(a) { return a + 1; }
main() { return my_func2(3); }
"

echo OK

