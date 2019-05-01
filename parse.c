#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hkdcc.h"

typedef struct {
  Vector *tokens;
  int pos;
} ParseState;

Token *cur_token(ParseState *state) {
  return (Token *)state->tokens->data[state->pos];
}
#define CUR_TOKEN ((Token *)state->tokens->data[state->pos])
#define INCR_POS state->pos++

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
  fprintf(stderr, "unexpected token at %d: %s\n", state->pos, CUR_TOKEN->input);
  exit(1);
}

// mul:  term
// mul:  term "*" mul
// mul:  term "/" mul
Node *mul(ParseState *state) {
  Node *lhs = term(state);
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

// expr:  mul expr'
// expr': ε | "+" expr | "-" expr | "==" expr | "!=" expr
Node *expr(ParseState *state) {
  Node *lhs = mul(state);
  if (CUR_TOKEN->type == '+') {
    INCR_POS; // skip +
    Node *rhs = expr(state);
    return new_node('+', lhs, rhs);
  }
  if (CUR_TOKEN->type == '-') {
    INCR_POS; // skip -
    Node *rhs = expr(state);
    return new_node('-', lhs, rhs);
  }
  if (CUR_TOKEN->type == TK_EQEQ) {
    INCR_POS; // skip TK_EQEQ
    Node *rhs = expr(state);
    return new_node(ND_EQEQ, lhs, rhs);
  }
  if (CUR_TOKEN->type == TK_NEQ) {
    INCR_POS; // skip TK_NEQ
    Node *rhs = expr(state);
    return new_node(ND_NEQ, lhs, rhs);
  }
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

  if (CUR_TOKEN->type == TK_SCOLON) { // ε
    if (!rhs) {
      return lhs;
    }
    return new_node(ND_ASGN, lhs, rhs);
  }
  fprintf(stderr, "unexpected token at %d: %s\n", state->pos, CUR_TOKEN->input);
  exit(1);
}

// statement: "return" assign ";"
// statement: assign ";"
Node *statement(ParseState *state) {
  if (CUR_TOKEN->type == TK_RETURN) {
    INCR_POS; // skip "return"
    Node *asgn = assign(state);
    Node *ret = new_node(ND_RET, asgn, NULL);
    INCR_POS; // skip ";"
    return ret;
  }
  Node *asgn = assign(state);
  INCR_POS; // skip ";"
  return asgn;
}

Vector *variable_names(Vector *nodes);

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

  Node *func_body = new_node(ND_FUNC_BODY, NULL, NULL);

  Vector *names = variable_names(statements);
  func_body->variable_names = names;
  func_body->statements = statements;
  return func_body;
}

// func_decl: ident "(" param ")" "{" func_body "}"
Node *func_decl(ParseState *state) {
  if (CUR_TOKEN->type != TK_IDENT) {
    return NULL;
  }
  char *ident = CUR_TOKEN->input;
  INCR_POS; // skip ident
  if (CUR_TOKEN->type != TK_LPAREN) {
    fprintf(stderr, "unexpected token at %d: expect ( but got %s\n", state->pos,
            CUR_TOKEN->input);
    exit(1);
  }
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

void show_node(Node *node, int indent) {
  for (int i = 0; i < indent; i++)
    printf(" ");

  switch (node->type) {
  case ND_NUM:
    printf("ND_NUM: %d\n", node->value);
    return;
  case ND_ASGN:
    printf("ND_ASGN:\n");
  default:
    printf("%c\n", node->type);
  }

  if (node->lhs)
    show_node(node->lhs, indent + 2);
  if (node->rhs)
    show_node(node->rhs, indent + 2);
}

void put_variable_name_on_node(Vector *v, Node *node) {
  if (node == NULL) {
    return;
  }
  put_variable_name_on_node(v, node->lhs);
  put_variable_name_on_node(v, node->rhs);
  if (node->type == ND_IDENT) {
    for (int i = v->len - 1; i >= 0; i--)
      if (strcmp(v->data[i], node->name) == 0)
        return;
    vec_push(v, node->name);
  }
}

Vector *variable_names(Vector *nodes) {
  Vector *v = new_vector();
  for (int i = 0; i < nodes->len; i++) {
    put_variable_name_on_node(v, nodes->data[i]);
  }
  return v;
}
