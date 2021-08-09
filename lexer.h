#pragma once

#include <stdbool.h>

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

typedef struct {
    TokenType type;
    char *text;
} Token;

typedef struct {
    Token *prev_token;
    Token *token;
    size_t pos;
    char *source;
    size_t source_len;
} Lexer;

char *token_type_name(TokenType type);

Lexer *lexer_create(char *source);

bool lexer_has_more_chars(Lexer *lexer);

char *substr(char *orig, size_t from, size_t to);

void lexer_scan(Lexer *lexer);
