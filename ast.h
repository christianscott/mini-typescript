#pragma once

#include <stddef.h>

typedef struct {
    size_t pos;
} Location;

typedef enum {
    EXPR_IDENT,
    EXPR_NUM,
    EXPR_ASSIGNMENT,
} ExprType;

typedef struct Expr_ Expr;

typedef struct {
    char *text;
    int id;
} Ident;

typedef struct {
    double value;
} Number;

typedef struct {
    Ident name;
    Expr *expr;
} Assignment;

struct Expr_ {
    ExprType type;
    Location location;

    union {
        // EXPR_IDENT
        Ident ident;
        // EXPR_NUM
        Number num;
        // EXPR_ASSIGNMENT
        Assignment assignment;
    };
};

typedef enum {
    DECL_LET,
    DECL_TYPE_ALIAS,
} DeclType;

typedef struct {
    Ident name;
    Ident *type_name;
    Expr init;
} Let;

typedef struct {
    Ident name;
    Ident type_name;
} TypeAlias;

typedef struct {
    DeclType type;
    Location location;

    union {
        // DECL_LET
        Let let;
        // DECL_TYPE_ALIAS
        TypeAlias type_alias;
    };
} Decl;

typedef enum {
    STMT_EXPR,
    STMT_DECL,
} StmtType;

typedef struct {
    StmtType type;
    Location location;

    union {
        // STMT_EXPR
        Expr expr;
        // STMT_DECL
        Decl decl;
    };
} Stmt;

typedef struct {
    char *id;
} Type;

Ident ident_create(char *text);

Expr expr_ident_create(Location location, char *text);

Expr expr_num_create(Location location, double value);

Expr expr_assignment_create(Location location, Ident name, Expr *value);

Decl decl_let_create(Location location, Ident name, Ident *type_name, Expr init);

Decl decl_type_alias_create(Location location, Ident name, Ident type_name);

Stmt stmt_expr_create(Location location, Expr expr);

Stmt stmt_decl_create(Location location, Decl decl);

