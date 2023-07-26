#ifndef BUILTIN_COMMANDS_H
#define BUILTIN_COMMANDS_H

extern const char *builtin_func_names[];
extern int (*builtin_functions[])(char **);

int sh_cd(char **args);
int sh_help();
int sh_exit();
int sh_sizeof_funcs();

#endif // BUILTIN_COMMANDS_H