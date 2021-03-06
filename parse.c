#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hkdcc.h"

typedef struct {
  Vector *tokens;
  int pos;
  Map *declared_variables;
  // TODO: declared functions
} ParseState;

#define CUR_TOKEN ((Token *)state->tokens->data[state->pos])
#define INCR_POS state->pos++

int declared_p(ParseState *state, char *name) {
  void *declared = map_get(state->declared_variables, name);
  return declared != NULL;
}

void add_variable_declaration(ParseState *state, char *name) {
  if (declared_p(state, name)) {
    fprintf(stderr, "already declared variable %s at %d\n", name, state->pos);
    exit(1);
  }
  int index = state->declared_variables->keys->len;
  Variable *var = malloc(sizeof(Variable));
  var->name = name;
  var->index = index;
  map_put(state->declared_variables, name, var);
}

void show_declared_variables(ParseState *state) {
  for (int i = 0; i < state->declared_variables->keys->len; i++) {
    char *k = state->declared_variables->keys->data[i];
    Variable *v = map_get(state->declared_variables, k);
    printf("%d: %s\n", i, v->name);
  }
}

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
  node->lhs = NULL;
  node->rhs = NULL;
  return node;
}

Node *new_node_ident(char *var) {
  Node *node = malloc(sizeof(Node));
  node->type = ND_IDENT;
  node->name = var;
  node->lhs = NULL;
  node->rhs = NULL;
  return node;
}

Node *expr();

// arguments: ε | expr "," arguments
Node *arguments(ParseState *state, int *argc) {
  if (CUR_TOKEN->type == TK_RPAREN) { // ()
    return NULL;
  }
  Node *lhs = expr(state);
  *argc = *argc + 1;

  if (CUR_TOKEN->type == TK_RPAREN) {
    return new_node(ND_ARGS, lhs, NULL);
  }
  if (CUR_TOKEN->type != TK_COMMA) {
    fprintf(stderr, "missing comma after an argument at %d\n", state->pos);
    exit(1);
  }
  INCR_POS; // skip ","
  Node *rhs = arguments(state, argc);
  return new_node(ND_ARGS, lhs, rhs);
}

// term: number | ident | ident "(" arguments ")"
// term: "(" expr ")"
Node *term(ParseState *state) {
  int beg = state->pos;
  if (CUR_TOKEN->type == TK_NUM) {
    Node *ret = new_node_num(CUR_TOKEN->value);
    INCR_POS;
    return ret;
  }
  if (CUR_TOKEN->type == TK_IDENT) {
    char *name = CUR_TOKEN->input;
    INCR_POS;
    if (CUR_TOKEN->type != TK_LPAREN) {
      // variable reference
      // TODO: function pointer...
      // ex: int foo() {return 1;} int main() { void* p = foo; }
      if (!declared_p(state, name)) {
        fprintf(stderr, "undeclared variable %s at %d\n", name, state->pos);
        exit(1);
      }
      Node *ret = new_node_ident(name);
      return ret;
    }
    int beg = state->pos;
    INCR_POS; // skip "("

    int argc = 0;
    Node *args = arguments(state, &argc);

    if (CUR_TOKEN->type == TK_RPAREN) {
      // function call
      Node *ret = new_node_ident(name); // TODO: fix this abuse
      ret->type = ND_CALL;
      ret->lhs = args;
      ret->value = argc;
      INCR_POS; // skip ")"
      return ret;
    }
    fprintf(stderr, "mismatch paren for function call, begin at %d, now %d\n",
            beg, state->pos);
    exit(1);
  }
  if (CUR_TOKEN->type == TK_LPAREN) {
    INCR_POS; // skip (
    Node *ret = expr(state);
    if (CUR_TOKEN->type == TK_RPAREN) {
      INCR_POS; // skip )
      return ret;
    }
    fprintf(stderr, "mismatch paren, begin at %d, now %d\n", beg, state->pos);
    exit(1);
  }
  fprintf(stderr, "[term]unexpected token at %d: %s\n", state->pos,
          CUR_TOKEN->input);
  exit(1);
}

// unary: term | "+" term | "-" term
Node *unary(ParseState *state) {
  if (CUR_TOKEN->type == '+') {
    INCR_POS; // skip "+"
    Node *t = term(state);
    return t;
  }
  if (CUR_TOKEN->type == '-') {
    INCR_POS; // skip "-"
    Node *lhs = new_node_num(0);
    Node *rhs = term(state);
    return new_node('-', lhs, rhs);
  }

  Node *t = term(state);
  return t;
}

// mul: unary
// mul: unary "*" mul
// mul: unary "/" mul
Node *mul(ParseState *state) {
  Node *lhs = unary(state);
  if (CUR_TOKEN->type == '*') {
    INCR_POS; // skip *
    Node *rhs = mul(state);
    return new_node('*', lhs, rhs);
  }
  if (CUR_TOKEN->type == '/') {
    INCR_POS; // skip /
    Node *rhs = mul(state);
    return new_node('/', lhs, rhs);
  }
  return lhs;
}

// add: mul | mul "+" add | mul "-" add
Node *add(ParseState *state) {
  Node *lhs = mul(state);
  if (CUR_TOKEN->type == '+') {
    INCR_POS; // skip +
    Node *rhs = add(state);
    return new_node('+', lhs, rhs);
  }
  if (CUR_TOKEN->type == '-') {
    INCR_POS; // skip -
    Node *rhs = add(state);
    return new_node('-', lhs, rhs);
  }
  return lhs;
}
// relational: add
//           | add "<" add
//           | add "<=" add
//           | add ">" add
//           | add ">=" add
Node *relational(ParseState *state) {
  Node *lhs = add(state);
  if (CUR_TOKEN->type == TK_LT) {
    INCR_POS; // skip "<"
    Node *rhs = add(state);
    return new_node(ND_LT, lhs, rhs);
  }
  if (CUR_TOKEN->type == TK_LTEQ) {
    INCR_POS; // skip "<="
    Node *rhs = relational(state);
    return new_node(ND_LTEQ, lhs, rhs);
  }
  if (CUR_TOKEN->type == TK_GT) {
    INCR_POS; // skip ">"
    Node *rhs = add(state);
    return new_node(ND_LT, rhs, lhs); // swapped
  }
  if (CUR_TOKEN->type == TK_GTEQ) {
    INCR_POS; // skip "="
    Node *rhs = relational(state);
    return new_node(ND_LTEQ, rhs, lhs); // swapped
  }
  return lhs;
}
// equality': relational
//          | relational "==" relational
//          | relational "!=" relational
Node *equality_tail(ParseState *state) {
  Node *lhs = relational(state);
  if (CUR_TOKEN->type == TK_EQEQ) {
    INCR_POS; // skip "=="
    Node *rhs = relational(state);
    return new_node(ND_EQEQ, lhs, rhs);
  }
  if (CUR_TOKEN->type == TK_NEQ) {
    INCR_POS; // skip "!="
    Node *rhs = relational(state);
    return new_node(ND_NEQ, lhs, rhs);
  }
  return lhs;
}
// equality: equality'
//         | equality' "==" relational
//         | equality' "!=" relational
Node *equality(ParseState *state) {
  Node *lhs = equality_tail(state);
  if (CUR_TOKEN->type == TK_EQEQ) {
    INCR_POS; // skip "=="
    Node *rhs = relational(state);
    return new_node(ND_EQEQ, lhs, rhs);
  }
  if (CUR_TOKEN->type == TK_NEQ) {
    INCR_POS; // skip "!="
    Node *rhs = relational(state);
    return new_node(ND_NEQ, lhs, rhs);
  }
  return lhs;
}

// expr: equality
Node *expr(ParseState *state) {
  Node *lhs = equality(state);
  return lhs;
}

// assign': ε | "=" expr assign'
Node *assign_tail(ParseState *state) {
  if (CUR_TOKEN->type != TK_EQ) { // ε
    return NULL;
  }
  INCR_POS; // skip "="
  Node *lhs = expr(state);
  Node *rhs = assign_tail(state);
  if (!rhs) {
    return lhs;
  }
  return new_node(ND_ASGN, lhs, rhs);
}

// assign : expr assign'
// assign': ε | "=" expr assign'
Node *assign(ParseState *state) {
  Node *lhs = expr(state);
  Node *rhs = assign_tail(state);

  if (!rhs) {
    return lhs;
  }
  return new_node(ND_ASGN, lhs, rhs);
}

// block_body: ε | statement block_body
// statement: "return" assign ";"
//          | "if" "(" assign ")" statement
//          | "while" "(" assign ")" statement
//          | "int" ident ";"
//          | "{" block_body "}"
//          | assign ";"
Node *statement(ParseState *state) {
  if (CUR_TOKEN->type == TK_RETURN) {
    INCR_POS; // skip "return"
    Node *asgn = assign(state);
    Node *ret = new_node(ND_RET, asgn, NULL);
    INCR_POS; // skip ";"
    return ret;
  }
  if (CUR_TOKEN->type == TK_IF) {
    INCR_POS; // skip "if"
    if (CUR_TOKEN->type != TK_LPAREN) {
      fprintf(stderr, "unexpected token at %d, expect ( but got %s\n",
              state->pos, CUR_TOKEN->input);
      exit(1);
    }
    INCR_POS; // skip "("
    Node *asgn = assign(state);
    if (CUR_TOKEN->type != TK_RPAREN) {
      fprintf(stderr, "unexpected token at %d, expect ) but got %s\n",
              state->pos, CUR_TOKEN->input);
      exit(1);
    }
    INCR_POS; // skip ")"
    Node *stmt = statement(state);
    Node *ret = new_node(ND_IF, asgn, stmt);
    return ret;
  }
  if (CUR_TOKEN->type == TK_WHILE) {
    INCR_POS; // skip "while"
    if (CUR_TOKEN->type != TK_LPAREN) {
      fprintf(stderr, "unexpected token at %d, expect ( but got %s\n",
              state->pos, CUR_TOKEN->input);
      exit(1);
    }
    INCR_POS; // skip "("
    Node *asgn = assign(state);
    if (CUR_TOKEN->type != TK_RPAREN) {
      fprintf(stderr, "unexpected token at %d, expect ) but got %s\n",
              state->pos, CUR_TOKEN->input);
      exit(1);
    }
    INCR_POS; // skip ")"
    Node *stmt = statement(state);
    Node *ret = new_node(ND_WHILE, asgn, stmt);
    return ret;
  }
  if (CUR_TOKEN->type == TK_IDENT && (strcmp(CUR_TOKEN->input, "int") == 0)) {
    INCR_POS; // skip "int"
    if (CUR_TOKEN->type != TK_IDENT) {
      fprintf(stderr, "unexpected token at %d, expect IDENTIFIER but got %s\n",
              state->pos, CUR_TOKEN->input);
      exit(1);
    }
    Node *ret = new_node(ND_VAR_DECL, NULL, NULL);
    ret->name = CUR_TOKEN->input;
    INCR_POS; // skip ident
    // TODO: support `int a, b;` and `int a = 1;`
    if (CUR_TOKEN->type != TK_SCOLON) {
      fprintf(stderr, "unexpected token at %d, expect ; but got %s\n",
              state->pos, CUR_TOKEN->input);
      exit(1);
    }
    add_variable_declaration(state, ret->name);
    INCR_POS; // skip ";"
    return ret;
  }

  // block
  if (CUR_TOKEN->type == TK_LBRACE) {
    INCR_POS; // skip "{"
    Vector *statements = new_vector();
    while (1) {
      if (CUR_TOKEN->type == TK_RBRACE) {
        break;
      }

      Node *stmt = statement(state);
      vec_push(statements, stmt);
    }
    INCR_POS; // skip "}"
    Node *block = new_node(ND_BLOCK, NULL, NULL);
    block->statements = statements;
    return block;
  }

  Node *asgn = assign(state);
  INCR_POS; // skip ";"
  return asgn;
}

// func_body: stmt func_body'
// func_body': ε | stmt func_body'
Node *func_body(ParseState *state) {
  Vector *statements = new_vector();
  while (1) {
    if (CUR_TOKEN->type == TK_RBRACE) {
      break;
    }

    Node *stmt = statement(state);
    vec_push(statements, stmt);
  }
  state->declared_variables = NULL;

  Node *func_body = new_node(ND_FUNC_BODY, NULL, NULL);

  func_body->statements = statements;
  return func_body;
}

// func_decl: "int" ident "(" param ")" "{" func_body "}"
Node *func_decl(ParseState *state) {
  if (CUR_TOKEN->type != TK_IDENT) {
    return NULL;
  }

  if ((strcmp(CUR_TOKEN->input, "int")) != 0) {
    return NULL;
  }

  INCR_POS; // skip "int"

  char *ident = CUR_TOKEN->input;
  INCR_POS; // skip ident
  if (CUR_TOKEN->type != TK_LPAREN) {
    fprintf(stderr, "unexpected token at %d: expect ( but got %s\n", state->pos,
            CUR_TOKEN->input);
    exit(1);
  }

  Map *variables = new_map();
  state->declared_variables = variables;

  INCR_POS; // skip "("

  Vector *parameters = new_vector();

  while (1) {
    if (CUR_TOKEN->type == TK_RPAREN) {
      break;
    }

    if (CUR_TOKEN->type != TK_IDENT) {
      fprintf(stderr, "unexpected token at %d: expect identifier but got %s\n",
              state->pos, CUR_TOKEN->input);
    }
    if (strcmp(CUR_TOKEN->input, "int") != 0) {
      fprintf(stderr, "unexpected token at %d: expect int but got %s\n",
              state->pos, CUR_TOKEN->input);
    }
    INCR_POS; // skip "int"

    if (CUR_TOKEN->type != TK_IDENT) {
      fprintf(stderr, "unexpected token at %d: expect identifier but got %s\n",
              state->pos, CUR_TOKEN->input);
      exit(1);
    }

    if (declared_p(state, CUR_TOKEN->input)) {
      fprintf(stderr, "already declared argument %s\n", CUR_TOKEN->input);
      exit(1);
    }
    add_variable_declaration(state, CUR_TOKEN->input);
    vec_push(parameters, CUR_TOKEN->input);
    INCR_POS; // skip IDENT

    if (CUR_TOKEN->type == TK_COMMA) {
      INCR_POS; // skip COMMA
    } else if (CUR_TOKEN->type == TK_RPAREN) {
      break;
    } else {
      fprintf(stderr, "unexpected token at %d: expect , or ) but got %s\n",
              state->pos, CUR_TOKEN->input);
      exit(1);
    }
  }

  Node *lhs = new_node(ND_FUNC_DECL, NULL, NULL);
  lhs->parameters = parameters;

  INCR_POS; // skip ")"
  if (CUR_TOKEN->type != TK_LBRACE) {
    fprintf(stderr, "unexpected token at %d: expect { but got %s\n", state->pos,
            CUR_TOKEN->input);
    exit(1);
  }
  INCR_POS; // skip "{"

  Node *rhs = func_body(state);

  rhs->variables = variables;
  state->declared_variables = NULL;

  if (CUR_TOKEN->type != TK_RBRACE) {
    fprintf(stderr, "unexpected token at %d: expect } but got %s\n", state->pos,
            CUR_TOKEN->input);
    exit(1);
  }
  INCR_POS; // skip "}"

  Node *node = new_node_ident(ident);
  node->type = ND_FUNC;
  node->lhs = lhs;
  node->rhs = rhs;

  return node;
}

// program  : program'
// program' : ε | func_decl program'
Node *program(ParseState *state) {
  Vector *functions = new_vector();
  while (1) {
    if (CUR_TOKEN->type == TK_EOF) {
      break;
    }
    Node *func = func_decl(state);
    vec_push(functions, func);
  }
  Node *prog = new_node(ND_PROG, NULL, NULL);
  prog->functions = functions;
  return prog;
}

char *tokenize_eq(char *p, Vector *tokens) {
  if (*p != '=') {
    fprintf(stderr, "assert error: tokenize_eq should be called with =\n");
    exit(1);
  }

  p++; // skip "="

  if (*p == '=') { // ==
    Token *token = malloc(sizeof(Token));
    token->type = TK_EQEQ;
    token->input = p;
    p++;
    vec_push(tokens, token);
    return p;
  }

  // single eq, such as "a = b"
  Token *token = malloc(sizeof(Token));
  token->type = TK_EQ;
  token->input = p - 1;
  vec_push(tokens, token);
  return p;
}

char *tokenize_bang(char *p, Vector *tokens) {
  if (*p != '!') {
    fprintf(stderr, "assert error: tokenize_bang should be called with !\n");
    exit(1);
  }

  p++; // skip "!"

  if (*p == '=') { // !=
    Token *token = malloc(sizeof(Token));
    token->type = TK_NEQ;
    token->input = p;
    p++; // skip !=
    vec_push(tokens, token);
    return p;
  }

  // TODO: currently, "!" is only a part of "!=". After impl unary operator
  // "!", this is not an error.
  fprintf(stderr, "unexpected characters %s\n", p);
  exit(1);
}

char *tokenize_lt(char *p, Vector *tokens) {
  if (*p != '<') {
    fprintf(stderr, "assert error: tokenize_lt should be called with <\n");
    exit(1);
  }
  char *beg = p;
  p++; // skip "<"

  if (*p == '=') { // <=
    Token *token = malloc(sizeof(Token));
    token->type = TK_LTEQ;
    token->input = beg;
    p++; // skip =
    vec_push(tokens, token);
    return p;
  }

  Token *token = malloc(sizeof(Token));
  token->type = TK_LT;
  token->input = beg;
  vec_push(tokens, token);
  return p;
}

char *tokenize_gt(char *p, Vector *tokens) {
  if (*p != '>') {
    fprintf(stderr, "assert error: tokenize_lt should be called with >\n");
    exit(1);
  }
  char *beg = p;
  p++; // skip ">"

  if (*p == '=') { // >=
    Token *token = malloc(sizeof(Token));
    token->type = TK_GTEQ;
    token->input = beg;
    p++; // skip =
    vec_push(tokens, token);
    return p;
  }

  Token *token = malloc(sizeof(Token));
  token->type = TK_GT;
  token->input = beg;
  vec_push(tokens, token);
  return p;
}

int is_identifier_head(char c) {
  return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || c == '_';
}

int is_identifier_char(char c) {
  return is_identifier_head(c) || ('0' <= c && c <= '9');
}

Vector *tokenize(char *p) {
  Vector *ret = new_vector();
  while (*p) {
    if (*p == '=') {
      p = tokenize_eq(p, ret);
      continue;
    }

    if (*p == '!') {
      p = tokenize_bang(p, ret);
      continue;
    }

    if (*p == '<') {
      p = tokenize_lt(p, ret);
      continue;
    }

    if (*p == '>') {
      p = tokenize_gt(p, ret);
      continue;
    }

    if (isspace(*p)) {
      p++;
      continue;
    }

    // semicolon
    if (*p == ';') {
      Token *token = malloc(sizeof(Token));
      token->type = TK_SCOLON;
      token->input = p;
      p++;
      vec_push(ret, token);
      continue;
    }
    // paren
    if (*p == '(') {
      Token *token = malloc(sizeof(Token));
      token->type = TK_LPAREN;
      token->input = p;
      p++;

      vec_push(ret, token);
      continue;
    }
    if (*p == ')') {
      Token *token = malloc(sizeof(Token));
      token->type = TK_RPAREN;
      token->input = p;
      p++;

      vec_push(ret, token);
      continue;
    }
    if (*p == '{') {
      Token *token = malloc(sizeof(Token));
      token->type = TK_LBRACE;
      token->input = p;
      p++;

      vec_push(ret, token);
      continue;
    }
    if (*p == '}') {
      Token *token = malloc(sizeof(Token));
      token->type = TK_RBRACE;
      token->input = p;
      p++;

      vec_push(ret, token);
      continue;
    }
    if (*p == ',') {
      Token *token = malloc(sizeof(Token));
      token->type = TK_COMMA;
      token->input = p;
      p++;

      vec_push(ret, token);
      continue;
    }

    // operator
    if (*p == '+' || *p == '-' || *p == '*' || *p == '/') {
      Token *token = malloc(sizeof(Token));
      token->type = *p;
      token->input = p;
      p++;
      vec_push(ret, token);
      continue;
    }

    // digit
    if (isdigit(*p)) {
      Token *token = malloc(sizeof(Token));
      token->type = TK_NUM;
      token->input = p;
      token->value = strtol(p, &p, 10);
      vec_push(ret, token);
      continue;
    }

    // identifier
    if (is_identifier_head(*p)) {
      char *beg = p;
      while (is_identifier_char(*p))
        p++;
      char *end = p;
      int size = end - beg;

      if (size == 6 && strncmp(beg, "return", 6) == 0) {
        Token *token = malloc(sizeof(Token));
        token->type = TK_RETURN;
        token->input = beg;
        vec_push(ret, token);
        continue;
      }

      if (size == 2 && strncmp(beg, "if", 2) == 0) {
        Token *token = malloc(sizeof(Token));
        token->type = TK_IF;
        token->input = beg;
        vec_push(ret, token);
        continue;
      }

      if (size == 5 && strncmp(beg, "while", 5) == 0) {
        Token *token = malloc(sizeof(Token));
        token->type = TK_WHILE;
        token->input = beg;
        vec_push(ret, token);
        continue;
      }

      char *s = malloc(sizeof(char) * (size + 1));
      memcpy(s, beg, size);
      s[size] = '\0';
      Token *token = malloc(sizeof(Token));
      token->type = TK_IDENT;
      token->input = s;
      vec_push(ret, token);
      continue;
    }

    fprintf(stderr, "unexpected characters %s\n", p);
    exit(1);
  }
  Token *token = malloc(sizeof(Token));
  token->type = TK_EOF;
  token->input = p;
  vec_push(ret, token);
  return ret;
}

Node *parse(Vector *tokens) {
  ParseState *state = malloc(sizeof(ParseState));
  state->tokens = tokens;
  state->pos = 0;
  state->declared_variables = NULL;
  Node *prog = program(state);

  free(state);
  return prog;
}

#define SHOW_TOKEN_CASE(name)                                                  \
  case name:                                                                   \
    if (token->input == NULL)                                                  \
      printf("%10s:\n", #name);                                                \
    else                                                                       \
      printf("%10s: %s\n", #name, token->input);                               \
    break;

void show_tokens(Vector *tokens) {
  for (int i = 0; i < tokens->len; i++) {
    Token *token = tokens->data[i];
    switch (token->type) {
      SHOW_TOKEN_CASE(TK_NUM)
      SHOW_TOKEN_CASE(TK_LPAREN)
      SHOW_TOKEN_CASE(TK_RPAREN)
      SHOW_TOKEN_CASE(TK_IDENT)
      SHOW_TOKEN_CASE(TK_SCOLON)
      SHOW_TOKEN_CASE(TK_EQ)
      SHOW_TOKEN_CASE(TK_EQEQ)
      SHOW_TOKEN_CASE(TK_NEQ)
      SHOW_TOKEN_CASE(TK_LTEQ)
      SHOW_TOKEN_CASE(TK_LT)
      SHOW_TOKEN_CASE(TK_GTEQ)
      SHOW_TOKEN_CASE(TK_GT)
      SHOW_TOKEN_CASE(TK_COMMA)
      SHOW_TOKEN_CASE(TK_LBRACE)
      SHOW_TOKEN_CASE(TK_RBRACE)
      SHOW_TOKEN_CASE(TK_RETURN)
      SHOW_TOKEN_CASE(TK_EOF)
      SHOW_TOKEN_CASE(TK_IF)
    default:
      printf("%10c:\n", token->type);
      break;
    }
  }
}

#define SHOW_NODE_CASE(name)                                                   \
  case name:                                                                   \
    printf("%10s:\n", #name);                                                  \
    break;

void show_node(Node *node, int indent) {
  for (int i = 0; i < indent; i++)
    printf(" ");

  switch (node->type) {
  case ND_NUM:
    printf("%10s: %d\n", "ND_NUM", node->value);
    return;
  case ND_IDENT:
    printf("%10s: %s\n", "ND_IDENT", node->name);
    return;
  case ND_PROG:
    printf("%10s:\n", "ND_PROG");
    for (int i = 0; i < node->functions->len; i++) {
      Node *tmp = node->functions->data[i];
      show_node(tmp, indent + 2);
    }
    return;
  case ND_FUNC:
    printf("%10s: %s\n", "ND_FUNC", node->name);
    break;
  case ND_FUNC_DECL:
    printf("%10s:\n", "ND_FUNC_DECL");
    for (int i = 0; i < node->parameters->len; i++) {
      Node *tmp = node->parameters->data[i];
      show_node(tmp, indent + 2);
    }
    return;
  case ND_FUNC_BODY:
    printf("%10s:\n", "ND_FUNC_BODY");
    for (int i = 0; i < node->statements->len; i++) {
      Node *tmp = node->statements->data[i];
      show_node(tmp, indent + 2);
    }
    return;
    SHOW_NODE_CASE(ND_ASGN);
    SHOW_NODE_CASE(ND_EQEQ);
    SHOW_NODE_CASE(ND_NEQ);
    SHOW_NODE_CASE(ND_LTEQ);
    SHOW_NODE_CASE(ND_LT);
    SHOW_NODE_CASE(ND_CALL);
    SHOW_NODE_CASE(ND_RET);
    SHOW_NODE_CASE(ND_IF);
    SHOW_NODE_CASE(ND_WHILE);
  default:
    printf("%10c:\n", node->type);
  }

  if (node->lhs)
    show_node(node->lhs, indent + 2);
  if (node->rhs)
    show_node(node->rhs, indent + 2);
}
