#!/bin/bash
build/hkdcc "
my_func2(a) { a + 1; }
main() { my_func2(3); }
"

echo OK

