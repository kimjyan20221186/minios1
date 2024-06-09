//kernel.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>

void print_minios(const char* str);
void handle_dir_command();

int main() {
    print_minios("[MiniOS SSU] Hello, World!");

    char *input;
    while(1) {
        input = readline("커맨드를 입력하세요(종료:exit) : ");

        if (input == NULL) {
            break;
        }

        if (strcmp(input, "exit") == 0) {
            free(input);
            break;
        }

        // 입력된 명령어를 공백으로 분리
        if (strcmp(input, "minisystem") == 0) {
            minisystem();
        } else if (strcmp(input, "dir") == 0) {
            handle_dir_command();
        } else system(input);

        free(input);
    }

    print_minios("[MiniOS SSU] MiniOS Shutdown........");

    return 1;
}

void handle_dir_command() {
    dir_main(); 
}
void print_minios(const char* str) {
    printf("%s\n", str);
}

