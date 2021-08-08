#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vendor/stretchy_buffer.h"

typedef enum {
    TOK_FUNCTION,
    TOK_LET,
    TOK_TYPE,
    TOK_RETURN,
    TOK_EQ,
    TOK_NUMBER,
    TOK_IDENT,
    TOK_SEMICOLON,
    TOK_COLON,
    TOK_END_OF_FILE,
    TOK_UNKNOWN,
} TokenType;

char *token_type_name(TokenType type) {
    switch (type) {
        case TOK_FUNCTION:
            return "TOK_FUNCTION";
        case TOK_LET:
            return "TOK_LET";
        case TOK_TYPE:
            return "TOK_TYPE";
        case TOK_RETURN:
            return "TOK_RETURN";
        case TOK_EQ:
            return "TOK_EQ";
        case TOK_NUMBER:
            return "TOK_NUMBER";
        case TOK_IDENT:
            return "TOK_IDENT";
        case TOK_SEMICOLON:
            return "TOK_SEMICOLON";
        case TOK_COLON:
            return "TOK_COLON";
        case TOK_END_OF_FILE:
            return "TOK_END_OF_FILE";
        case TOK_UNKNOWN:
            return "TOK_UNKNOWN";
        default:
            return "(unknown token type)";
    }
}

typedef struct {
    TokenType type;
    char *text;
} Token;

Token *token_create(TokenType type, char *text) {
    Token *token = malloc(sizeof(Token));
    token->type = type;
    token->text = text;
    return token;
}

typedef struct {
    Token *token;
    size_t pos;
    char *source;
    size_t source_len;
} Lexer;

Lexer *lexer_create(char *source) {
    Lexer *lexer = malloc(sizeof(Lexer));
    lexer->token = NULL;
    lexer->pos = 0;
    lexer->source = source;
    lexer->source_len = strlen(source);
    return lexer;
}

bool lexer_has_more_chars(Lexer *lexer) {
    return lexer->pos < lexer->source_len;
}

char lexer_char(Lexer *lexer) {
    return lexer->source[lexer->pos];
}

char *substr(char *orig, size_t from, size_t to) {
    size_t len = to - from;
    char *sub = malloc(sizeof(char) * (len + 1));
    strncpy(sub, orig + from, len);
    sub[len] = '\0';
    return sub;
}

bool is_digit(char c) {
    return '0' <= c && c <= '9';
}

bool is_alpha(char c) {
    return ('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z');
}

bool is_alphanumeric(char c) {
    return is_alpha(c) || is_digit(c);
}

bool is_identifier_char(char c) {
    return is_alphanumeric(c) || c == '_';
}

void lexer_scan(Lexer *lexer) {
    if (lexer->token != NULL && lexer->token->type == TOK_END_OF_FILE) {
        return;
    }

    while (lexer_has_more_chars(lexer) && isspace(lexer_char(lexer))) {
        lexer->pos++;
    }

    size_t start = lexer->pos;
    if (!lexer_has_more_chars(lexer)) {
        lexer->token = token_create(TOK_END_OF_FILE, "EOF");
        return;
    }

    if (is_digit(lexer_char(lexer))) {
        while (lexer_has_more_chars(lexer) && is_digit(lexer_char(lexer))) {
            lexer->pos++;
        }

        char *text = substr(lexer->source, start, lexer->pos);
        lexer->token = token_create(TOK_NUMBER, text);
        return;
    }

    if (is_alpha(lexer_char(lexer))) {
        while (lexer_has_more_chars(lexer) && is_identifier_char(lexer_char(lexer))) {
            lexer->pos++;
        }

        char *text = substr(lexer->source, start, lexer->pos);
        TokenType type;
        if (strcmp(text, "function") == 0) {
            type = TOK_FUNCTION;
        } else if (strcmp(text, "let") == 0) {
            type = TOK_LET;
        } else if (strcmp(text, "type") == 0) {
            type = TOK_TYPE;
        } else if (strcmp(text, "return") == 0) {
            type = TOK_RETURN;
        } else {
            type = TOK_IDENT;
        }
        lexer->token = token_create(type, text);
        return;
    }

    lexer->pos++;
    switch (lexer->source[lexer->pos - 1]) {
        case '=':
            lexer->token = token_create(TOK_EQ, "=");
            break;
        case ';':
            lexer->token = token_create(TOK_SEMICOLON, ";");
            break;
        case ':':
            lexer->token = token_create(TOK_COLON, ":");
            break;
        default: {
            char *text = substr(lexer->source, start, lexer->pos);
            lexer->token = token_create(TOK_UNKNOWN, text);
            break;
        }
    }
}

typedef struct {
    size_t pos;
} Location;

typedef enum {
    EXPR_IDENT,
    EXPR_NUM,
    EXPR_ASSIGNMENT,
} ExprType;

typedef struct {
    char *text;
} Ident;

typedef struct {
    double value;
} Number;

typedef struct Expr_ Expr;

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

Expr expr_ident_create(Location location, char *text) {
    Expr expr;
    expr.type = EXPR_IDENT;
    expr.location = location;
    Ident ident = {.text = text};
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

typedef struct {
    Decl *value_decl;
    Decl *decls;
} Symbol;

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

typedef struct {
    char *id;
} Type;

typedef struct {
    Stmt *statements;
} Module;

typedef struct {
    Lexer *lexer;
    bool has_errors;
} Parser;

Parser *parser_create(Lexer *lexer) {
    Parser *parser = malloc(sizeof(Parser));
    parser->lexer = lexer;
    parser->has_errors = false;
    return parser;
}

bool parser_try_parse_token(Parser *parser, TokenType type) {
    bool ok = parser->lexer->token != NULL && parser->lexer->token->type == type;
    if (ok) {
        lexer_scan(parser->lexer);
    }
    return ok;
}

void parser_expect_token(Parser *parser, TokenType type) {
    bool ok = parser_try_parse_token(parser, type);
    if (!ok) {
        fprintf(stderr, "expected a token of type %s, got %s\n", token_type_name(type),
                token_type_name(parser->lexer->token->type));
        parser->has_errors = true;
    }
}

Expr parse_identifier_or_literal(Parser *parser) {
    size_t pos = parser->lexer->pos;
    Location location = {.pos = pos};
    if (parser_try_parse_token(parser, TOK_IDENT)) {
        return expr_ident_create(location, parser->lexer->token->text);
    }

    if (parser_try_parse_token(parser, TOK_NUMBER)) {
        char *end_ptr;
        double value = strtod(parser->lexer->token->text, &end_ptr);
        if (errno == ERANGE) {
            fprintf(stderr, "failed to parse as double: %s\n", parser->lexer->token->text);
            parser->has_errors = true;
        }
        return expr_num_create(location, value);
    }

    fprintf(stderr, "expected identifier but got a literal\n");
    parser->has_errors = true;
    lexer_scan(parser->lexer);

    Expr expr = expr_ident_create(location, "(missing)");
    return expr;
}

Expr parse_expression(Parser *parser) {
    size_t pos = parser->lexer->pos;
    Location location = {.pos = pos};

    Expr expr = parse_identifier_or_literal(parser);
    if (expr.type == EXPR_IDENT && parser_try_parse_token(parser, TOK_EQ)) {
        Expr *value = malloc(sizeof(Expr));
        *value = parse_expression(parser);
        return expr_assignment_create(location, expr.ident, value);
    }
    return expr;
}

Ident parse_identifier(Parser *parser) {
    Expr expr = parse_identifier_or_literal(parser);
    if (expr.type == EXPR_IDENT) {
        return expr.ident;
    }

    fprintf(stderr, "expected identifier but got a literal\n");
    parser->has_errors = true;
    Ident ident = {
            .text = "(missing)",
    };
    return ident;
}

Stmt parse_stmt(Parser *parser) {
    size_t pos = parser->lexer->pos;
    Location location = {.pos = pos};

    if (parser_try_parse_token(parser, TOK_LET)) {
        Ident name = parse_identifier(parser);
        Ident *type_name = NULL;
        if (parser_try_parse_token(parser, TOK_COLON)) {
            type_name = malloc(sizeof(Ident));
            *type_name = parse_identifier(parser);
        }
        parser_expect_token(parser, TOK_EQ);
        Expr init = parse_expression(parser);
        Decl decl = decl_let_create(location, name, type_name, init);
        return stmt_decl_create(location, decl);
    }

    if (parser_try_parse_token(parser, TOK_TYPE)) {
        Ident name = parse_identifier(parser);
        parser_expect_token(parser, TOK_EQ);
        Ident type_name = parse_identifier(parser);
        Decl decl = decl_type_alias_create(location, name, type_name);
        return stmt_decl_create(location, decl);
    }

    Expr expr = parse_expression(parser);
    return stmt_expr_create(location, expr);
}

Module *parser_parse_module(Parser *parser) {
    Stmt *statements = NULL;
    Module *mod = malloc(sizeof(Module));
    mod->statements = statements;

    lexer_scan(parser->lexer);
    if (parser_try_parse_token(parser, TOK_END_OF_FILE)) {
        return mod;
    }

    sb_push(statements, parse_stmt(parser));
    while (parser_try_parse_token(parser, TOK_SEMICOLON) && parser->lexer->token->type != TOK_END_OF_FILE) {
        sb_push(statements, parse_stmt(parser));
    }
    parser_expect_token(parser, TOK_END_OF_FILE);

    return mod;
}

Module *parser_parse(Parser *parser) {
    return parser_parse_module(parser);
}

int main() {
    char *source = "let a = 1;\n"
                   "let b: number = 2;\n"
                   "let c = a = b;";
    Parser *parser = parser_create(lexer_create(source));
    parser_parse(parser);
    if (parser->has_errors) {
        printf("something's not quite right\n");
        return 1;
    }
    printf("all ok\n");
    return 0;
}
