#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

enum {
    TK_DIG = 256, // digit
    TK_EOF,
};

typedef struct {
    int type;
    int value;
    char* input;
} Token;

// Buffer for tokens.
// up to 100 tokens for now...
Token tokens[100];

void tokenize(char* p) {
    int idx = 0;
    while(*p) {
        if (isspace(*p)) {
            p++;
            continue;
        }

        // operator
        if (*p == '+' || *p == '-') {
            tokens[idx].type = *p;
            tokens[idx].input = p;
            p++;
            idx++;
            continue;
        }

        // digit
        if (isdigit(*p)) {
            tokens[idx].type = TK_DIG;
            tokens[idx].input = p;
            tokens[idx].value = strtol(p, &p, 10);
            idx++;
            continue;
        }

        fprintf(stderr, "unexpected characters %s\n", p);
        exit(1);
    }
    tokens[idx].type = TK_EOF;
    tokens[idx].input = p;
}

// rax: return value
int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "arguments count mismatch\n");
        return 1;
    }

    tokenize(argv[1]);

    printf(".intel_syntax noprefix\n");
    printf(".global _main\n");
    printf("_main:\n");

    if (tokens[0].type != TK_DIG) {
        // TODO: error handling
        exit(1);
    }
    printf("  mov rax, %d\n", tokens[0].value);

    int i = 1;
    while(tokens[i].type != TK_EOF) {
        switch (tokens[i].type) {
            case '+':
                if (tokens[i+1].type != TK_DIG) {
                    // TODO: error handling
                    exit(1);
                }
                printf("  add rax, %d\n", tokens[i+1].value);
                i += 2;
                break;
            case '-':
                if (tokens[i+1].type != TK_DIG) {
                    // TODO: error handling
                    exit(1);
                }
                printf("  sub rax, %d\n", tokens[i+1].value);
                i += 2;
                break;
            default:
                fprintf(stderr, "unexpected token at %d: %s\n", i, tokens[i].input);
                return 1;
        }
    }

    printf("  ret\n");
    return 0;
}
