#include <stdio.h>
#include <stdlib.h>
#include <wait.h>
#include <memory.h>
#include <sys/types.h>
#include <unistd.h>

#define JOBS 512
#define BUFSIZE 1024
#define W_BUFSIZE 64
#define TRNA " \t\r\n\a"

typedef struct Job {
    int pid;
    int length;
    char **argv;
} Job;

Job *jobs[JOBS];

/*
  Function Declarations.
 */
int cd(char **args);

int shell_exit(char **args);

void printJobs();

char *readLine();

char **parse(char *line);

int execute(char **args);

void loop();

int launch(char **args, int backGround, int length);

void addJob(char** args, int length, int pid);
/*
 * Function to read input from command line.
 */
char *readLine() {
    int bufferSize = BUFSIZE;
    int position = 0;
    char *buffer = (char *) malloc(sizeof(char) * bufferSize);
    int c;

    if (!buffer) {
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
    }

    while (1) {
        // Read a character
        c = getchar();

        // If we hit EOF, replace it with a null character and return.
        if (c == EOF || c == '\n') {
            buffer[position] = '\0';
            break;
        } else {
            buffer[position] = c;
        }
        position++;

        // If we have exceeded the buffer, reallocate.
        if (position >= bufferSize) {
            bufferSize += BUFSIZE;
            buffer = (char *) realloc(buffer, bufferSize);
            if (!buffer) {
                fprintf(stderr, "lsh: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }
    }

    return buffer;
}
/*
 * Function to split user input and put into array.
 */
char **parse(char *line) {
    int bufsize = W_BUFSIZE, position = 0;
    char **words = malloc(bufsize * sizeof(char *));
    char *token;

    if (!words) {
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
    }

    token = strtok(line, TRNA);
    while (token != NULL) {
        words[position] = token;
        position++;

        if (position >= bufsize) {
            bufsize += W_BUFSIZE;
            words = realloc(words, bufsize * sizeof(char *));
            if (!words) {
                fprintf(stderr, "lsh: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }

        token = strtok(NULL, TRNA);
    }
    words[position] = NULL;
    return words;
}
/*
 * Function to run user command.
 */
int launch(char **args, int backGround, int length) {
    pid_t pid, wpid;
    int status, i;

    pid = fork();
    if (pid == 0) {
        // Child process
        if (execvp(args[0], args) == -1) {
            perror("Child process error");
        }
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        // Error forking
        perror("Error forking");
    } else {
        printf("%d\n", pid);
        if (backGround == 1) {
            // Parent process
            do {
                wpid = waitpid(pid, &status, WUNTRACED);
            } while (!WIFEXITED(status) && !WIFSIGNALED(status));
        } else {
            // Add background
            addJob(args,length,pid);
        }
    }

    return 1;
}

/*
  List of builtin commands, followed by their corresponding functions.
 */
char *builtin_str[] = {"cd", "exit"};

int (*builtin_func[])(char **) = {&cd, &shell_exit};

int builtins() {
    return sizeof(builtin_str) / sizeof(char *);
}

/*
  Builtin function implementations.
*/
int cd(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "lsh: expected argument to \"cd\"\n");
    } else {
        if (chdir(args[1]) != 0) {
            perror("lsh");
        }
    }
    return 1;
}

int shell_exit(char **args) {
    return 0;
}

void printJobs() {
    int i, j, k;
    for (i = 0; i < JOBS; i++) {
        if (jobs[i] != NULL) {
            if (waitpid(jobs[i]->pid, NULL, WNOHANG != 0)) {
                for (k = 0; k < jobs[i]->length; k++) {
                    free(jobs[i]->argv[k]);
                }
                free(jobs[i]->argv);
                free(jobs[i]);
                jobs[i] = NULL;
            } else {
                printf("%d ", jobs[i]->pid);
                for (j = 0; j < jobs[i]->length; j++) {
                    printf("%s ", jobs[i]->argv[j]);
                }
                printf("\n");
            }
        }
    }
}

/*
 * Function to execute.
 */
int execute(char **args) {
    int i = 0;
    int length = 0;

    if (args[0] == NULL) {
        // An empty command was entered.
        return 1;
    }

    while (args[i] != NULL) {
        length++;
        i++;
    }

    for (i = 0; i < builtins(); i++) {
        if (strcmp(args[0], builtin_str[i]) == 0) {
            return (*builtin_func[i])(args);
        }
    }

    if (strcmp(args[0], "jobs") == 0) {
        printJobs();
        return 1;
    }


    if (strcmp(args[length - 1], "&") == 0) {
        // Run in background.
        args[length - 1] = NULL;
        return launch(args, 0, length - 1);
    }

    return launch(args, 1, length);
}

void loop() {
    char *line;
    char **args;
    int status;


    do {
        printf("%s", "> ");
        // Read command from user.
        line = readLine();
        // Parse line.
        args = parse(line);
        // Execute commands.
        status = execute(args);

        // Free memory.
        free(line);
        free(args);

    } while (status);

}

/*
 * Function to add job to jobs list.
 */
void addJob(char** args, int length, int pid){
    int i;
    Job *currentJob = (Job *) malloc(sizeof(Job));
    currentJob->argv = malloc(BUFSIZE * sizeof(char *));
    for (i = 0; i < length; i++) {
        currentJob->argv[i] = (char *) malloc(sizeof(char) * BUFSIZE);
        strcpy(currentJob->argv[i], args[i]);
    }
    currentJob->length = length;
    currentJob->pid = pid;
    i=0;
    while (jobs[i]!= NULL){
        i++;
    }
    jobs[i] = currentJob;

}

/*
 * main Function.
 */
int main() {
    // Load config.
    for (int i = 0; i < JOBS; ++i) {
        jobs[i] = NULL;
    }
    // Run REPL loop.
    loop();

    return 0;
}