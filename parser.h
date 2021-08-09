#pragma once

#include "bind.h"
#include "lexer.h"

typedef struct {
    Lexer *lexer;
    bool has_errors;
} Parser;

typedef enum {
    PARSE_RESULT_OK,
    PARSE_RESULT_UNEXPECTED_TOK,
    PARSE_RESULT_INVALID_NUMERIC_LITERAL,
} ParseResult;

char *parse_result_name(ParseResult res);

Parser *parser_create(Lexer *lexer);

ParseResult parser_parse(Parser *parser, Module *module);
