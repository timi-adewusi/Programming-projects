#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

/* Three output buffers: headers, function definitions, main() body */
char headers[2000], funcs[20000], main_body[20000];
char *target  = main_body;  /* points to whichever buffer we're filling */
int  indent   = 1;          /* current indentation depth                 */
int  in_function = 0;       /* are we inside a fn block?                 */

/* String variable registry — so `say` knows to use %s vs %lld */
char str_vars[100][32];
int  str_count = 0;

/* Symbol Table to store our definitions */
typedef struct { char name[32]; char value[64]; } Symbol;
Symbol symbols[100];
int sym_count = 0;

/* Global variable registry — variables shared between main and functions */
char global_vars[100][32];
int  global_count = 0;

void add_global_var(char *name) {
    for (int i = 0; i < global_count; i++)
        if (!strcmp(global_vars[i], name)) return;  /* already added */
    strcpy(global_vars[global_count++], name);
}

int is_global_var(char *name) {
    for (int i = 0; i < global_count; i++)
        if (!strcmp(global_vars[i], name)) return 1;
    return 0;
}

/* Replace defined symbols in a line */
void replace_symbols(char *line) {
    for (int i = 0; i < sym_count; i++) {
        char *p;
        while ((p = strstr(line, symbols[i].name))) {
            size_t len = strlen(symbols[i].name);
            char tail[256]; strcpy(tail, p + len);
            sprintf(p, "%s%s", symbols[i].value, tail);
        }
    }
}

int is_str(char *name) {
    for (int i = 0; i < str_count; i++)
        if (!strcmp(str_vars[i], name)) return 1;
    return 0;
}

/* Append an indented, formatted line to the current target buffer */
void emit(int lvl, const char *fmt, ...) {
    char tmp[512];
    va_list ap; va_start(ap, fmt); vsnprintf(tmp, sizeof tmp, fmt, ap); va_end(ap);
    char *p = target + strlen(target);
    sprintf(p, "%*s%s", lvl * 4, "", tmp);
}

void compile_line(char *raw) {
    char *l = raw;
    while (isspace(*l)) l++;
    if (*l == '\0' || *l == '#') return;
    l[strcspn(l, "\n")] = '\0';  /* strip trailing newline */

    char tok[64] = {0};
    sscanf(l, "%63s", tok);

    if (!strcmp(tok, "def")) {
        sscanf(l, "def %s = %s", symbols[sym_count].name, symbols[sym_count].value);
        sym_count++;
        return;
    }

    /* Replace any symbols in the line before processing */
    char processed[256];
    strcpy(processed, l);
    replace_symbols(processed);
    l = processed;
    sscanf(l, "%63s", tok);  /* re-read tok after symbol replacement */

    if (!strcmp(tok, "use")) {
        char mod[64]; sscanf(l, "use %63s", mod);
        sprintf(headers + strlen(headers), "#include <%s.h>\n", mod);

    } else if (!strcmp(tok, "fn")) {
        in_function = 1;
        char name[64]; sscanf(l, "fn %63s", name);
        /* strip trailing '{' or whitespace from name */
        name[strcspn(name, " {")] = '\0';
        target = funcs; indent = 1;
        sprintf(target + strlen(target), "void %s() {\n", name);

    } else if (!strcmp(tok, "call")) {
        char name[64]; sscanf(l, "call %63s", name);
        emit(indent, "%s();\n", name);

    } else if (!strcmp(tok, "let")) {
        char varname[64] = {0};
        sscanf(l + 4, "%63s", varname);
        /* strip any trailing '=' that got merged in */
        varname[strcspn(varname, "=")] = '\0';

        int already_global = is_global_var(varname);

        char *eq = strchr(l, '=');
        if (eq) {
            char *rhs = eq + 1;
            while (isspace(*rhs)) rhs++;

            if (rhs[0] == '@' && rhs[1] == '(') {
                char *close = strchr(rhs + 2, ')');
                if (close) {
                    char addr_expr[128];
                    strncpy(addr_expr, rhs + 2, close - rhs - 2);
                    addr_expr[close - rhs - 2] = '\0';
                    if (already_global)
                        emit(indent, "%s = *((long long*)(%s));\n", varname, addr_expr);
                    else
                        emit(indent, "long long %s = *((long long*)(%s));\n", varname, addr_expr);
                }
            } else if (strstr(rhs, "malloc") || strstr(rhs, "calloc")) {
                if (already_global)
                    emit(indent, "%s = (long long)%s;\n", varname, rhs);
                else
                    emit(indent, "long long %s = (long long)%s;\n", varname, rhs);
            } else {
                if (already_global)
                    emit(indent, "%s = %s;\n", varname, rhs);
                else
                    emit(indent, "long long %s = %s;\n", varname, rhs);
            }
        } else {
            if (already_global)
                emit(indent, "%s = 0;\n", varname);
            else
                emit(indent, "long long %s = 0;\n", varname);
        }

    } else if (!strcmp(tok, "listen")) {
        char var[64]; sscanf(l, "listen %63s", var);
        strcpy(str_vars[str_count++], var);
        emit(indent, "char %s[256]; scanf(\"%%255s\", %s);\n", var, var);

    } else if (!strcmp(tok, "ask")) {
        char var[64]; sscanf(l, "ask %63s", var);
        emit(indent, "scanf(\"%%lld\", &%s);\n", var);

    } else if (!strcmp(tok, "say")) {
        char *val = l + 4;
        while (isspace(*val)) val++;
        char vname[64]; sscanf(val, "%63s", vname);
        if (val[0] == '"' || is_str(vname))
            emit(indent, "printf(\"%%s\\n\", %s);\n", val);
        else
            emit(indent, "printf(\"%%lld\\n\", %s);\n", val);

    } else if (!strcmp(tok, "loop") || !strcmp(tok, "if")) {
        char cond[128] = {0};
        sscanf(l + strlen(tok), " %127[^{]", cond);
        char *end = cond + strlen(cond) - 1;
        while (end > cond && isspace(*end)) *end-- = '\0';
        emit(indent, "%s (%s) {\n", !strcmp(tok, "loop") ? "while" : "if", cond);
        indent++;

    } else if (!strcmp(tok, "else")) {
        char *p = target + strlen(target);
        if (p > target) p--;         /* '\n' */
        if (p > target) p--;         /* '}'  */
        while (p > target && (*(p-1) == ' ' || *(p-1) == '\t')) p--;
        *p = '\0';
        emit(indent, "} else {\n");
        indent++;

    } else if (!strcmp(tok, "}")) {
        indent--;
        emit(indent, "}\n");
        if (indent == 0) {
            target = main_body;
            indent = 1;
            in_function = 0;
        }

    } else if (!strcmp(tok, "stop")) {
        emit(indent, "break;\n");

    } else {
        /* free() — needs void* cast */
        if (strstr(l, "free(")) {
            char var[64];
            if (sscanf(l, "free(%63[^)])", var) == 1) {
                emit(indent, "free((void*)%s);\n", var);
                return;
            }
        }

        /* RHS dereference: var = @(expr) */
        char *eq = strchr(l, '=');
        if (eq) {
            char *rhs = eq + 1;
            while (isspace(*rhs)) rhs++;
            if (rhs[0] == '@' && rhs[1] == '(') {
                char *close = strchr(rhs + 2, ')');
                if (close) {
                    char varname[64], addr_expr[128];
                    strncpy(varname, l, eq - l);
                    varname[eq - l] = '\0';
                    /* trim trailing spaces from varname */
                    int vlen = strlen(varname);
                    while (vlen > 0 && isspace(varname[vlen-1])) varname[--vlen] = '\0';
                    strncpy(addr_expr, rhs + 2, close - rhs - 2);
                    addr_expr[close - rhs - 2] = '\0';
                    emit(indent, "%s = *((long long*)(%s));\n", varname, addr_expr);
                    return;
                }
            }
        }

        /* LHS dereference: @(addr) = val */
        if (l[0] == '@') {
            char *eq2 = strchr(l, '=');
            if (eq2) {
                char addr_expr[128], val[64];
                char *open  = strchr(l, '(');
                char *close = strchr(l, ')');
                if (open && close) {
                    strncpy(addr_expr, open + 1, close - open - 1);
                    addr_expr[close - open - 1] = '\0';
                    sscanf(eq2 + 1, " %63[^\n]", val);
                    emit(indent, "*((long long*)(%s)) = %s;\n", addr_expr, val);
                    return;
                }
            }
        }

        /* Default: pass through as raw C */
        emit(indent, "%s;\n", l);
    }
}

int main(int argc, char **argv) {
    if (argc < 2) { fprintf(stderr, "Usage: sc <file.sc>\n"); return 1; }
    FILE *in = fopen(argv[1], "r");
    if (!in) { perror(argv[1]); return 1; }

    char line[256];
    while (fgets(line, sizeof line, in)) compile_line(line);
    fclose(in);

    /* Find variables declared in main that are also used in functions,
       and promote them to globals so functions can access them. */
    char declared_vars[100][32];
    int  declared_count = 0;

    char main_copy[20000];
    strcpy(main_copy, main_body);
    char *p = main_copy;
    while ((p = strstr(p, "long long "))) {
        char varname[64];
        if (sscanf(p, "long long %63s", varname) == 1) {
            /* strip trailing punctuation */
            int len = strlen(varname);
            while (len > 0 && (varname[len-1] == '=' || varname[len-1] == ';' || varname[len-1] == ' '))
                varname[--len] = '\0';
            strcpy(declared_vars[declared_count++], varname);
        }
        p += 10;
    }

    for (int i = 0; i < declared_count; i++) {
        if (strstr(funcs, declared_vars[i])) {
            char search_str[128];
            sprintf(search_str, "long long %s", declared_vars[i]);
            if (!strstr(funcs, search_str)) {
                add_global_var(declared_vars[i]);
                /* Remove the local "long long varname" declaration from main_body
                   so the global is used instead (avoid shadowing). */
                char *decl = strstr(main_body, search_str);
                if (decl) {
                    size_t skip = strlen("long long ");
                    memmove(decl, decl + skip, strlen(decl + skip) + 1);
                }
            }
        }
    }

    FILE *out = fopen("output.c", "w");
    if (!out) { perror("output.c"); return 1; }

    /* Build global variable declarations */
    char globals_decl[5000] = {0};
    for (int i = 0; i < global_count; i++)
        sprintf(globals_decl + strlen(globals_decl), "long long %s = 0;\n", global_vars[i]);

    fprintf(out,
        "#include <stdio.h>\n"
        "#include <stdlib.h>\n"
        "%s\n"          /* extra headers (use ...) */
        "%s\n"          /* global var declarations  */
        "%s\n"          /* function definitions     */
        "int main() {\n"
        "%s\n"          /* main body                */
        "    return 0;\n"
        "}\n",
        headers, globals_decl, funcs, main_body);

    fclose(out);
    printf("Compiled to output.c\n");
    return 0;
}
