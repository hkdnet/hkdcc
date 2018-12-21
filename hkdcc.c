#include <stdio.h>
#include <stdlib.h>

// rax: return value
int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "arguments count mismatch\n");
        return 1;
    }

    char* p = argv[1];
    long l; // read buffer

    printf(".intel_syntax noprefix\n");
    printf(".global _main\n");
    printf("_main:\n");
    l = strtol(p, &p, 10);
    printf("  mov rax, %ld\n", l);

    while(*p) {
        if (*p == '+') {
            p++; // skip +
            l = strtol(p, &p, 10);
            printf("  add rax, %ld\n", l);
        } else if (*p == '-') {
            p++; // skip -
            l = strtol(p, &p, 10);
            printf("  sub rax, %ld\n", l);
        } else {
            fprintf(stderr, "Unexpected value %s\n", p);
            return 1;
        }
    }

    printf("  ret\n");
    return 0;
}
