#include "builtin_commands.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

const char *builtin_func_names[] = {"cd", "help", "exit"};
int (*builtin_functions[])(char **) = {&sh_cd, &sh_help, &sh_exit}; // Create a functions pointer array that has the 3 functions.

int sh_sizeof_funcs()
{
    return sizeof(builtin_func_names) / sizeof(char *);
}

int sh_help()
{
    int i;
    printf("Type program names and arguments, and hit enter.\n");
    printf("The following are built in:\n");

    for (i = 0; i < sh_sizeof_funcs(); i++)
    {
        printf(" - %s\n", builtin_func_names[i]);
    }

    printf("Use the man command for information on other programs.\n");
    return 1;
}

int sh_cd(char **args)
{
    if (args[1] == NULL)
    {
        fprintf(stderr, "jsh: Expected argument to \"cd\"\n");
        return EXIT_FAILURE;
    }
    else
    {
        if (chdir(args[1]) != 0)
        {
            fprintf(stderr, "jsh: Could not change directory.\n");
            return EXIT_FAILURE;
        }
    }

    return 1;
}

int sh_exit()
{
    return EXIT_SUCCESS;
}