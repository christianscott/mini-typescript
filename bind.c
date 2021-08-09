#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>

#include "bind.h"
#include "vendor/stretchy_buffer.h"

BindResult module_bind(Module *mod) {
    for (int i = 0; i < sb_count(mod->statements); i++) {
        Stmt stmt = mod->statements[i];
        if (stmt.type != STMT_DECL) {
            continue;
        }

        int id = stmt.decl.let.name.id;
        int n = sb_count(mod->locals);
        if (id > n) {
            for (int j = n - 1; j < (2 * n); j++) {
                LocalsEntry entry = {.set = false};
                sb_push(mod->locals, entry);
            }
        } else if (id == 0 && n == 0) {
            LocalsEntry entry = {.set = false};
            sb_push(mod->locals, entry);
        }

        if (mod->locals[i].set) {
            LocalsEntry entry = mod->locals[i];
            for (int j = 0; j < sb_count(entry.local.decls); j++) {
                Decl other = entry.local.decls[i];
                if (other.type == stmt.decl.type) {
                    fprintf(stderr,
                            "cannot redeclare %s; first declared at %zu\n",
                            stmt.decl.let.name.text,
                            other.location.pos);
                    return BIND_RESULT_CANNOT_REDECLARE;
                } else {
                    sb_push(entry.local.decls, stmt.decl);
                    if (stmt.decl.type == DECL_LET) {
                        entry.local.has_value_decl = true;
                        entry.local.value_decl = stmt.decl;
                    }
                }
            }
        } else {
            Symbol symbol = {.decls = NULL, .has_value_decl = false};
            Decl *decls = NULL;
            sb_push(decls, stmt.decl);
            if (stmt.decl.type == DECL_LET) {
                symbol.has_value_decl = true;
                symbol.value_decl = stmt.decl;
            }

            LocalsEntry entry = {.set = true, .local = symbol};
            mod->locals[i] = entry;
        }
    }

    return BIND_RESULT_OK;
}
