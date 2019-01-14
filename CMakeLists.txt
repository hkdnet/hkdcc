cmake_minimum_required(VERSION 3.10)
project(hkdcc C)
add_executable(hkdcc hkdcc.c codegen.c parse.c util.c util_test.c)
