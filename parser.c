#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "bind.h"
#include "lexer.h"
#include "parser.h"
#include "vendor/stretchy_buffer.h"

#define TRY_PARSE(__expr) \
    do { \
        ParseResult __res = __expr; \
        if (__res != PARSE_RESULT_OK) return __res; \
    } while (0)

#define PARSER_ERROR(...) \
    do {                  \
        if (parser->has_errors) break; \
        parser->has_errors = true; \
        parser_print_error_context(parser); \
        fprintf(stderr, __VA_ARGS__); \
    } while (0)

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
