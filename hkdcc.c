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
  int value; // the value of ND_NUM
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

Node *new_node_num(int value) {
  Node *node = malloc(sizeof(Node));
  node->type = ND_NUM;
  node->value = value;
  return node;
}

Node *expr();

// term: number
// term: "(" expr ")"
Node *term() {
  if (tokens[pos].type == TK_NUM) {
    return new_node_num(tokens[pos++].value);
  }
  if (tokens[pos].type == TK_LPAREN) {
    pos++;
    Node *ret = expr();
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

    // paren
    if (*p == '(') {
      tokens[idx].type = TK_LPAREN;
      tokens[idx].input = p;
      p++;
      idx++;
      continue;
    }
    if (*p == ')') {
      tokens[idx].type = TK_RPAREN;
      tokens[idx].input = p;
      p++;
      idx++;
      continue;
    }

    // operator
    if (*p == '+' || *p == '-' || *p == '*' || *p == '/') {
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

void generate(Node *node) {
  if (node->type == ND_NUM) {
    printf("  push %d\n", node->value);
    return;
  }
  if (node->lhs)
    generate(node->lhs);
  if (node->rhs)
    generate(node->rhs);

  // two operand
  printf("  pop rdi\n");
  printf("  pop rax\n");
  switch (node->type) {
  case '+':
    printf("  add rax, rdi\n");
    break;
  case '-':
    printf("  sub rax, rdi\n");
    break;
  case '*':
    printf("  mul rdi\n"); // memo: this means `rax = rax * rdi`
    break;
  case '/':
    printf("  mov rdx, 0\n");
    printf("  div rdi\n"); // memo: rax = ((rdx << 64) | rax) / rdi
    break;
  }
  printf("  push rax\n");
}

// rax: return value
// rsp: stack pointer
// rdi, rsi, rdx, rcs, r8, r9: args
int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "arguments count mismatch\n");
    return 1;
  }

  tokenize(argv[1]);

  printf(".intel_syntax noprefix\n");
  printf(".global _main\n");
  printf("_main:\n");

  Node *prog = expr();

  generate(prog);

  printf("  pop rax\n");
  printf("  ret\n");
  return 0;
}
