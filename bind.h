#pragma once

#include <stdbool.h>

#include "ast.h"

typedef struct {
    bool has_value_decl;
    Decl value_decl;
    Decl *decls;
} Symbol;

typedef struct {
    bool set;
    Symbol local;
} LocalsEntry;

typedef struct {
    Stmt *statements;
    LocalsEntry *locals;
} Module;

typedef enum {
    BIND_RESULT_OK,
    BIND_RESULT_CANNOT_REDECLARE,
} BindResult;

BindResult module_bind(Module *mod);
