typedef struct {
  void **data;
  int capacity;
  int len;
} Vector;

typedef struct {
  Vector *keys;
  Vector *values;
} Map;

enum {
  TK_NUM = 256, // number
  TK_LPAREN,    // (
  TK_RPAREN,    // )
  TK_IDENT,     // a-z
  TK_SCOLON,    // ;
  TK_EQ,        // =
  TK_EQEQ,      // ==
  TK_NEQ,       // !=
  TK_LTEQ,      // <=
  TK_LT,        // <
  TK_GTEQ,      // >=
  TK_GT,        // >
  TK_COMMA,     // ,
  TK_LBRACE,    // {
  TK_RBRACE,    // }
  TK_RETURN,    // "return"
  TK_IF,        // "if"
  TK_WHILE,     // "while"
  TK_EOF,
};

typedef struct {
  int type;
  int value;
  char *input;
} Token;

enum {
  ND_NUM = 256, // number node
  ND_IDENT,     // identifier
  ND_PROG,      // program
  ND_ASGN,
  ND_EQEQ,
  ND_NEQ,
  ND_LTEQ,
  ND_LT,
  ND_CALL,
  ND_ARGS,
  ND_FUNC,
  ND_FUNC_DECL,
  ND_FUNC_BODY,
  ND_RET,
  ND_IF,
  ND_WHILE,
  ND_VAR_DECL,
};

typedef struct Node {
  int type; // ND_X, + or -
  struct Node *lhs;
  struct Node *rhs;
  int value;  // the value of ND_NUM or argc for ND_ARGS
  char *name; // for ND_IDENT
  Vector *variable_names;
  union {
    Vector *statements; // for ND_FUNC_BODY
    Vector *parameters; // for ND_FUNC_DECL
    Vector *functions;  // for ND_PROG
  };
} Node;

// util.c
Vector *new_vector();

void vec_push(Vector *vec, void *elem);

Map *new_map();

void map_put(Map *map, void *key, void *value);

void *map_get(Map *map, char *key);

// util_test.c
void runtest();

// parser.c
Vector *tokenize(char *p);

Node *parse();

void show_tokens(Vector *tokens);

void show_node(Node *node, int indent);

// codegen.c
void generate(Node *node, Vector *var_names);
