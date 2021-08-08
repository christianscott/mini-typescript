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
    Token *prev_token;
    Token *token;
    size_t pos;
    char *source;
    size_t source_len;
} Lexer;

Lexer *lexer_create(char *source) {
    Lexer *lexer = malloc(sizeof(Lexer));
    lexer->token = NULL;
    lexer->prev_token = NULL;
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

void lexer_set_token(Lexer *lexer, Token *token) {
    if (lexer->prev_token != NULL) {
        free(lexer->prev_token);
    }
    lexer->prev_token = lexer->token;
    lexer->token = token;
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
        lexer_set_token(lexer, token_create(TOK_END_OF_FILE, "EOF"));
        return;
    }

    if (is_digit(lexer_char(lexer))) {
        while (lexer_has_more_chars(lexer) && is_digit(lexer_char(lexer))) {
            lexer->pos++;
        }

        char *text = substr(lexer->source, start, lexer->pos);
        lexer_set_token(lexer, token_create(TOK_NUMBER, text));
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
        lexer_set_token(lexer, token_create(type, text));
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

typedef enum {
    PARSE_RESULT_OK,
    PARSE_RESULT_UNEXPECTED_TOK,
    PARSE_RESULT_INVALID_NUMERIC_LITERAL,
} ParseResult;

char *parse_result_name(ParseResult res) {
    switch (res) {
        case PARSE_RESULT_OK:
            return "PARSE_RESULT_OK";
        case PARSE_RESULT_UNEXPECTED_TOK:
            return "PARSE_RESULT_UNEXPECTED_TOK";
        case PARSE_RESULT_INVALID_NUMERIC_LITERAL:
            return "PARSE_RESULT_INVALID_NUMERIC_LITERAL";
        default:
            return "(unknown)";
    }
}

#define TRY_PARSE(__expr) \
    do { \
        ParseResult __res = __expr; \
        if (__res != PARSE_RESULT_OK) return __res; \
    } while (0)

Parser *parser_create(Lexer *lexer) {
    Parser *parser = malloc(sizeof(Parser));
    parser->lexer = lexer;
    parser->has_errors = false;
    return parser;
}

void parser_print_error_context(Parser *parser) {
    size_t pos = parser->lexer->pos;

    size_t line_start = pos;
    size_t line_end = pos;
    while (parser->lexer->source[line_start - 1] != '\n' && line_start > 0) {
        line_start--;
    }
    while (parser->lexer->source[line_end] != '\n' && line_end < parser->lexer->source_len) {
        line_end++;
    }

    char *current_line = substr(parser->lexer->source, line_start, line_end + 1);
    fprintf(stderr, "%s", current_line);

    size_t padding_size = pos - line_start - 1;
    char *padding = malloc(sizeof(char) * padding_size);
    for (size_t i = 0; i < padding_size; i++) {
        padding[i] = ' ';
    }

    fprintf(stderr, "%s^ ", padding);
}

#define PARSER_ERROR(...) \
    do {                  \
        if (parser->has_errors) break; \
        parser->has_errors = true; \
        parser_print_error_context(parser); \
        fprintf(stderr, __VA_ARGS__); \
    } while (0)

bool parser_try_parse_token(Parser *parser, TokenType type) {
    bool ok = parser->lexer->token != NULL && parser->lexer->token->type == type;
    if (ok) {
        lexer_scan(parser->lexer);
    }
    return ok;
}

ParseResult parser_expect_token(Parser *parser, TokenType type) {
    bool ok = parser_try_parse_token(parser, type);
    if (!ok) {
        PARSER_ERROR("expected a token of type %s, got %s\n",
                     token_type_name(type),
                     token_type_name(parser->lexer->token->type));
        return PARSE_RESULT_UNEXPECTED_TOK;
    }
    return PARSE_RESULT_OK;
}

ParseResult parse_identifier_or_literal(Parser *parser, Expr *expr) {
    size_t pos = parser->lexer->pos;
    Location location = {.pos = pos};
    if (parser_try_parse_token(parser, TOK_IDENT)) {
        *expr = expr_ident_create(location, parser->lexer->token->text);
        return PARSE_RESULT_OK;
    }

    if (parser_try_parse_token(parser, TOK_NUMBER)) {
        char *end_ptr;
        double value = strtod(parser->lexer->token->text, &end_ptr);
        if (errno == ERANGE) {
            PARSER_ERROR("could not parse as double: %s\n", parser->lexer->token->text);
            return PARSE_RESULT_INVALID_NUMERIC_LITERAL;
        }
        *expr = expr_num_create(location, value);
        return PARSE_RESULT_OK;
    }

    PARSER_ERROR("expected identifier or a literal but got %s\n", token_type_name(parser->lexer->token->type));
    return PARSE_RESULT_UNEXPECTED_TOK;
}

ParseResult parse_expression(Parser *parser, Expr *expr) {
    size_t pos = parser->lexer->pos;
    Location location = {.pos = pos};

    TRY_PARSE(parse_identifier_or_literal(parser, expr));

    if (expr->type == EXPR_IDENT && parser_try_parse_token(parser, TOK_EQ)) {
        Expr *value = malloc(sizeof(Expr));
        TRY_PARSE(parse_expression(parser, value));
        *expr = expr_assignment_create(location, expr->ident, value);
    }

    return PARSE_RESULT_OK;
}

ParseResult parse_identifier(Parser *parser, Ident *ident) {
    Expr expr;
    TRY_PARSE(parse_identifier_or_literal(parser, &expr));
    if (expr.type == EXPR_IDENT) {
        *ident = expr.ident;
        return PARSE_RESULT_OK;
    }

    PARSER_ERROR("expected identifier but got a literal\n");
    return PARSE_RESULT_UNEXPECTED_TOK;
}

ParseResult parse_stmt(Parser *parser, Stmt *stmt) {
    size_t pos = parser->lexer->pos;
    Location location = {.pos = pos};

    if (parser_try_parse_token(parser, TOK_LET)) {
        // let $name: $type_name = $expr;
        Ident name;
        TRY_PARSE(parse_identifier(parser, &name));

        Ident *type_name = NULL;
        if (parser_try_parse_token(parser, TOK_COLON)) {
            type_name = malloc(sizeof(Ident));
            TRY_PARSE(parse_identifier(parser, type_name));
        }

        TRY_PARSE(parser_expect_token(parser, TOK_EQ));

        Expr init;
        TRY_PARSE(parse_expression(parser, &init));
        Decl decl = decl_let_create(location, name, type_name, init);
        *stmt = stmt_decl_create(location, decl);
    } else if (parser_try_parse_token(parser, TOK_TYPE)) {
        // type $name = $type_name;
        Ident name;
        TRY_PARSE(parse_identifier(parser, &name));

        TRY_PARSE(parser_expect_token(parser, TOK_EQ));

        Ident type_name;
        TRY_PARSE(parse_identifier(parser, &type_name));

        Decl decl = decl_type_alias_create(location, name, type_name);
        *stmt = stmt_decl_create(location, decl);
    } else {
        // $expr;
        Expr expr;
        TRY_PARSE(parse_expression(parser, &expr));
        *stmt = stmt_expr_create(location, expr);
    }

    TRY_PARSE(parser_expect_token(parser, TOK_SEMICOLON));
    return PARSE_RESULT_OK;
}

void parser_synchronize(Parser *parser) {
    lexer_scan(parser->lexer);

    while (!lexer_has_more_chars(parser->lexer)) {
        Token *prev_token = parser->lexer->prev_token;
        if (prev_token != NULL && prev_token->type == TOK_SEMICOLON) {
            return;
        }

        switch (parser->lexer->token->type) {
            case TOK_LET:
            case TOK_FUNCTION:
            case TOK_TYPE:
            case TOK_RETURN:
                return;
            default:
                break;
        }

        lexer_scan(parser->lexer);
    }
}

ParseResult parser_parse_module(Parser *parser, Module *mod) {
    lexer_scan(parser->lexer);
    if (parser_try_parse_token(parser, TOK_END_OF_FILE)) {
        return PARSE_RESULT_OK;
    }

    ParseResult res;
    while (true) {
        Stmt stmt;
        res = parse_stmt(parser, &stmt);
        if (res != PARSE_RESULT_OK) {
            parser_synchronize(parser);
            parser->has_errors = false;
        }
        sb_push(mod->statements, stmt);

        if (parser_try_parse_token(parser, TOK_END_OF_FILE)) {
            break;
        }
    }

    return res;
}

ParseResult parser_parse(Parser *parser, Module *module) {
    return parser_parse_module(parser, module);
}

int main(int argc, char **argv) {
    char *source;
    if (argc > 1) {
        source = argv[1];
    } else {
        source = "let a = 1;\n"
                 "let b: number = 2;\n"
                 "let c = a = b;";
    }

    Parser *parser = parser_create(lexer_create(source));

    Stmt *statements = NULL;
    Module *mod = malloc(sizeof(Module));
    mod->statements = statements;
    ParseResult res = parser_parse(parser, mod);

    if (res != PARSE_RESULT_OK) {
        printf("failed to parse: %s\n", parse_result_name(res));
        return 1;
    }

    return 0;
}
