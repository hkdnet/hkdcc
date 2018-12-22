#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

enum {
  TK_LPAREN = 254, // (
  TK_RPAREN = 255, // )
  TK_NUM = 256,    // number
  TK_EOF,
};

typedef struct {
  int type;
  int value;
  char *input;
} Token;

enum {
  ND_NUM = 256, // number node
};

typedef struct Node {
  int type; // ND_X, + or -
  struct Node *lhs;
  struct Node *rhs;
  int val; // the value of ND_NUM
} Node;

// Buffer for tokens.
// up to 100 tokens for now...
Token tokens[100];
int pos = 0;

Node *new_node(int type, Node *lhs, Node *rhs) {
  Node *node = malloc(sizeof(Node));
  node->type = type;
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
}

Node *new_node_num(int val) {
  Node *node = malloc(sizeof(Node));
  node->type = ND_NUM;
  node->val = val;
  return node;
}

// term: number
// term: "(" expr ")"
Node *term() {
  if (tokens[pos].type == TK_NUM) {
    return new_node_num(tokens[pos++].value);
  }
  if (tokens[pos].type == TK_LPAREN) {
    pos++;
    Node *ret = term();
    if (tokens[pos].type == TK_RPAREN) {
      return ret;
    }
    fprintf(stderr, "mismatch paren\n");
    exit(1);
  }
  fprintf(stderr, "unexpected token at %d: %s\n", pos, tokens[pos].input);
  exit(1);
}

// mul:  term
// mul:  term "*" mul
// mul:  term "/" mul
Node *mul() {
  Node *lhs = term();
  if (tokens[pos].type == '*') {
    pos++; // skip *
    Node *rhs = mul();
    return new_node('*', lhs, rhs);
  }
  if (tokens[pos].type == '/') {
    pos++; // skip /
    Node *rhs = mul();
    return new_node('/', lhs, rhs);
  }
  return lhs;
}

// expr:  mul expr'
// expr': ε | "+" expr | "-" expr
Node *expr() {
  Node *lhs = mul();
  if (tokens[pos].type == '+') {
    pos++; // skip +
    Node *rhs = expr();
    return new_node('+', lhs, rhs);
  }
  if (tokens[pos].type == '-') {
    pos++; // skip -
    Node *rhs = expr();
    return new_node('-', lhs, rhs);
  }
  return lhs;
}

void tokenize(char *p) {
  int idx = 0;
  while (*p) {
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
      tokens[idx].type = TK_NUM;
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

void token_error(int i) {
  fprintf(stderr, "unexpected token at %d: %s\n", i, tokens[i].input);
  exit(1);
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

  if (tokens[0].type != TK_NUM) {
    token_error(0);
  }
  printf("  mov rax, %d\n", tokens[0].value);

  int i = 1;
  while (tokens[i].type != TK_EOF) {
    switch (tokens[i].type) {
    case '+':
      if (tokens[i + 1].type != TK_NUM) {
        token_error(i);
      }
      printf("  add rax, %d\n", tokens[i + 1].value);
      i += 2;
      break;
    case '-':
      if (tokens[i + 1].type != TK_NUM) {
        token_error(i);
      }
      printf("  sub rax, %d\n", tokens[i + 1].value);
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
