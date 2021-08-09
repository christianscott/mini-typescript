//
// Created by Christian Scott on 9/8/21.
//

#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>

#include "lexer.h"

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

Token *token_create(TokenType type, char *text) {
    Token *token = malloc(sizeof(Token));
    token->type = type;
    token->text = text;
    return token;
}

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
