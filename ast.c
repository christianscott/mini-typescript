#include "ast.h"

static int ident_next_id_ = 0;
int ident_next_id() {
    return ident_next_id_++;
}

Ident ident_create(char *text) {
    Ident ident = { .text = text, .id = ident_next_id() };
    return ident;
}

Expr expr_ident_create(Location location, char *text) {
    Expr expr;
    expr.type = EXPR_IDENT;
    expr.location = location;
    Ident ident = ident_create(text);
    expr.ident = ident;
    return expr;
}

Expr expr_num_create(Location location, double value) {
    Expr expr;
    expr.type = EXPR_NUM;
    expr.location = location;
    Number num = {.value = value};
    expr.num = num;
    return expr;
}

Expr expr_assignment_create(Location location, Ident name, Expr *value) {
    Assignment assignment = {.expr = value, .name = name};
    Expr expr = {
            .type = EXPR_ASSIGNMENT,
            .location = location,
            .assignment = assignment,
    };
    return expr;
}

Decl decl_let_create(Location location, Ident name, Ident *type_name, Expr init) {
    Decl decl;
    decl.type = DECL_LET;
    decl.location = location;

    Let let = {.name = name, .type_name = type_name, .init = init};
    decl.let = let;

    return decl;
}

Decl decl_type_alias_create(Location location, Ident name, Ident type_name) {
    Decl decl;
    decl.type = DECL_TYPE_ALIAS;
    decl.location = location;
    TypeAlias type_alias = {.name = name, .type_name = type_name};
    decl.type_alias = type_alias;
    return decl;
}

Stmt stmt_expr_create(Location location, Expr expr) {
    Stmt stmt;
    stmt.type = STMT_EXPR;
    stmt.location = location;
    stmt.expr = expr;
    return stmt;
}

Stmt stmt_decl_create(Location location, Decl decl) {
    Stmt stmt;
    stmt.type = STMT_DECL;
    stmt.location = location;
    stmt.decl = decl;
    return stmt;
}
