#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include "builtin_commands.h"
#include <stdbool.h>

#define BUFF_SIZE 1024
#define TOKEN_DELIM " \t\r\n\a"

void sh_loop();
char *sh_read_line();
char **sh_get_args(char *line);
int sh_launch(char **args);
int sh_execute(char **args);

int main(int arc, char **argv)
{
    // Run command loop.
    sh_loop();

    return EXIT_SUCCESS;
}

void sh_loop()
{
    int status;
    do
    {
        printf("> ");
        char *line = sh_read_line();
        char **args = sh_get_args(line);
        status = sh_execute(args);

        free(line);

        // Delete each argument since they are malloc-ed
        size_t index = 0;
        while (args[index] != NULL)
        {
            free(args[index]);
            index++;
        }
    } while (status);
}

char *sh_read_line()
{
    unsigned long long int buffersize = BUFF_SIZE;
    char *buffer = malloc(sizeof(char) * buffersize);
    char c;
    long int position = 0;

    if (buffer == NULL)
    {
        fprintf(stderr, "sh: Could not allocate memory for line buffer.\n");
        exit(EXIT_FAILURE);
    }

    while (1)
    {
        c = getchar();

        if (c == EOF || c == '\n')
        {
            buffer[position] = '\0';
            return buffer;
        }
        else
        {
            buffer[position] = c;
        }

        // Re-allocate buffer in case it doesn't fit the line.
        if (position >= buffersize)
        {
            buffersize = buffersize + (buffersize / 2);
            buffer = realloc(buffer, buffersize);

            if (buffer == NULL)
            {
                fprintf(stderr, "sh: Could not allocate memory for line buffer.\n");
                exit(EXIT_FAILURE);
            }
        }

        position++;
    }
}

char **sh_get_args(char *line)
{
    unsigned long long int buffersize = BUFF_SIZE;
    char **tokens = malloc(BUFF_SIZE * sizeof(char *));
    size_t position = 0;
    bool inside_quotes = false;

    if (tokens == NULL)
    {
        fprintf(stderr, "sh: Could not allocate memory for the tokens.\n");
        exit(EXIT_FAILURE);
    }

    char *token = strtok(line, TOKEN_DELIM);
    while (token != NULL)
    {
        if (inside_quotes)
        {
            // Check if the token ends with an unescaped quote
            size_t len = strlen(token);
            if (len > 1 && token[len - 1] == '"' && token[len - 2] != '\\')
            {
                // Remove the ending quote from the token
                token[len - 1] = '\0';
                inside_quotes = false;
            }

            // Append the token to the previous one (separated by a space)
            size_t last_len = strlen(tokens[position - 1]);
            tokens[position - 1] = realloc(tokens[position - 1], last_len + strlen(token) + 2); // +2 for space and null terminator
            strcat(tokens[position - 1], " ");
            strcat(tokens[position - 1], token);
        }
        else
        {
            // Check if the token starts with a quote
            if (token[0] == '"')
            {
                // Remove the starting quote from the token
                token = &token[1];
                inside_quotes = true;
            }

            tokens[position] = strdup(token);
            position++;
        }

        // Increase buffer size
        if (position >= buffersize)
        {
            buffersize = buffersize + (buffersize / 2);
            tokens = realloc(tokens, buffersize * sizeof(char *));

            if (tokens == NULL)
            {
                fprintf(stderr, "sh: Could not allocate memory for the tokens.\n");
                exit(EXIT_FAILURE);
            }
        }

        token = strtok(NULL, TOKEN_DELIM);
    }

    tokens[position] = NULL;
    return tokens;
}

int sh_launch(char **args)
{
    bool needPipes = false;
    int *fd = malloc(sizeof(int) * 2);
    if (fd == NULL)
    {
        fprintf(stderr, "sh: Could not allocate memory for the pipe.\n");
        return 1;
    }

    int i = 0;

    while (args[i] != NULL)
    {
        if (strcmp(args[i], "|") == 0)
        {
            needPipes = true;
            break;
        }

        i++;
    }

    if (!needPipes)
    {
        free(fd);
    }
    else
    {
        if (pipe(fd) == -1)
        {
            fprintf(stderr, "sh: Could not create a pipe.\n");
            return 1;
        }
    }

    int status;
    int pid = fork();

    if (pid < 0)
    {
        fprintf(stderr, "sh: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    else if (pid == 0) // Child process
    {
        if (needPipes)
        {
            dup2(fd[1], STDOUT_FILENO);
            close(fd[0]);
            close(fd[1]);

            args[i] = NULL; // Set args[i] to NULL to terminate the argument list at the index i

            if (execvp(args[0], args) == -1)
            {
                fprintf(stderr, "sh: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            if (execvp(args[0], args) == -1)
            {
                fprintf(stderr, "sh: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }
        }
    }

    // The child process has been replaced. This is for the parent process.
    if (needPipes)
    {
        int pid2 = fork();

        if (pid2 < 0)
        {
            fprintf(stderr, "sh: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        else if (pid2 == 0) // Child process
        {
            dup2(fd[0], STDIN_FILENO);
            close(fd[0]);
            close(fd[1]);

            if (execvp(args[i + 1], args + i + 1) == -1)
            {
                fprintf(stderr, "sh: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }
        }

        waitpid(pid, NULL, WUNTRACED);
        waitpid(pid2, NULL, WUNTRACED);
        printf("\n");
    }
    else
    {
        // We didn't use pipes
        waitpid(pid, NULL, WUNTRACED);
        printf("\n");
    }

    return 1;
}

int sh_execute(char **args)
{
    if (args[0] == NULL)
    {
        // An empty command was entered.
        return EXIT_FAILURE;
    }

    for (size_t i = 0; i < sh_sizeof_funcs(); i++)
    {
        if (strcmp(args[0], builtin_func_names[i]) == 0)
        {
            return (*builtin_functions[i])(args);
        }
    }

    return sh_launch(args);
}