#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lexer.h"
#include "bind.h"
#include "parser.h"

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
    mod->locals = NULL;

    ParseResult res = parser_parse(parser, mod);
    if (res != PARSE_RESULT_OK) {
        printf("failed to parse: %s\n", parse_result_name(res));
        return 1;
    }

    BindResult bind_res = module_bind(mod);
    if (bind_res != BIND_RESULT_OK) {
        return 1;
    }

    return 0;
}
