#include "command.h"
#include "syscall.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#define BUFFER_SIZE 1024

struct linux_dirent {
    long           d_ino;
    off_t          d_off;
    unsigned short d_reclen;
    char           d_name[];
};

void ls(const char *path) {
    if (path == NULL || strlen(path) == 0) {
        path = ".";
    }
    int fd = syscall(SYS_OPEN, path, O_RDONLY | O_DIRECTORY);
    if (fd < 0) {
        perror("ls: open");
        return;
    }

    char buffer[BUFFER_SIZE];
    struct linux_dirent *d;
    int nread;

    while ((nread = syscall(SYS_GETDENTS, fd, buffer, sizeof(buffer))) > 0) {
        for (int bpos = 0; bpos < nread;) {
            d = (struct linux_dirent *) (buffer + bpos);
            printf("%s\n", d->d_name);
            bpos += d->d_reclen;
        }
    }
    if (nread == -1) {
        perror("ls: getdents");
    }
    syscall(SYS_CLOSE, fd);
}

void cd(const char *path) {
    if (syscall(SYS_CHDIR, path) != 0) {
        perror("cd");
    }
}

void pwd() {
    char cwd[BUFFER_SIZE];
    if (syscall(SYS_GETCWD, cwd, sizeof(cwd)) != -1) {
        printf("%s\n", cwd);
    } else {
        perror("pwd");
    }
}

void make_dir(const char *path) {
    if (path == NULL || strlen(path) == 0) {
        fprintf(stderr, "mkdir: missing operand\n");
        return;
    }
    if (syscall(SYS_MKDIR, path, 0777) != 0) {
        perror("mkdir");
    }
}

void remove_dir(const char *path) {
    if (path == NULL || strlen(path) == 0) {
        fprintf(stderr, "rmdir: missing operand\n");
        return;
    }
    if (syscall(SYS_RMDIR, path) != 0) {
        perror("rmdir");
    }
}

void touch(const char *path) {
    int fd = syscall(SYS_OPEN, path, O_CREAT | O_WRONLY, 0666);
    if (fd < 0) {
        perror("touch");
    } else {
        syscall(SYS_CLOSE, fd);
    }
}

void rm(const char *path) {
    if (syscall(SYS_UNLINK, path) != 0) {
        perror("rm");
    }
}

void cp(const char *args) {
    char src[256], dest[256];
    sscanf(args, "%s %s", src, dest);
    int src_fd = syscall(SYS_OPEN, src, O_RDONLY);
    if (src_fd < 0) {
        perror("cp: open source");
        return;
    }

    int dest_fd = syscall(SYS_OPEN, dest, O_WRONLY | O_CREAT, 0666);
    if (dest_fd < 0) {
        perror("cp: open destination");
        syscall(SYS_CLOSE, src_fd);
        return;
    }

    char buffer[BUFFER_SIZE];
    ssize_t nread;
    while ((nread = syscall(SYS_READ, src_fd, buffer, sizeof(buffer))) > 0) {
        if (syscall(SYS_WRITE, dest_fd, buffer, nread) != nread) {
            perror("cp: write");
            syscall(SYS_CLOSE, src_fd);
            syscall(SYS_CLOSE, dest_fd);
            return;
        }
    }

    if (nread < 0) {
        perror("cp: read");
    }
    syscall(SYS_CLOSE, src_fd);
    syscall(SYS_CLOSE, dest_fd);
}

void mv(const char *args) {
    char src[256], dest[256];
    sscanf(args, "%s %s", src, dest);
    if (syscall(SYS_RENAME, src, dest) != 0) {
        perror("mv");
    }
}

void help() {
    printf("Available commands:\n");
    printf("  ls [path]       - List directory contents\n");
    printf("  cd [path]       - Change the current directory\n");
    printf("  pwd             - Print the current working directory\n");
    printf("  mkdir [path]    - Create a new directory\n");
    printf("  rmdir [path]    - Remove an empty directory\n");
    printf("  touch [path]    - Create a new empty file\n");
    printf("  rm [path]       - Remove a file\n");
    printf("  cp [src] [dest] - Copy a file\n");
    printf("  mv [src] [dest] - Move/rename a file\n");
    printf("  help            - Show this help message\n");
    printf("  exit            - Exit the shell\n");
}

void execute_command(const char *command, const char *args) {
    if (strcmp(command, "ls") == 0) {
        ls(args);
    } else if (strcmp(command, "cd") == 0) {
        cd(args);
    } else if (strcmp(command, "pwd") == 0) {
        pwd();
    } else if (strcmp(command, "mkdir") == 0) {
        make_dir(args);
    } else if (strcmp(command, "rmdir") == 0) {
        remove_dir(args);
    } else if (strcmp(command, "touch") == 0) {
        touch(args);
    } else if (strcmp(command, "rm") == 0) {
        rm(args);
    } else if (strcmp(command, "cp") == 0) {
        cp(args);
    } else if (strcmp(command, "mv") == 0) {
        mv(args);
    } else if (strcmp(command, "help") == 0) {
        help();
    } else {
        printf("Unknown command: %s\n", command);
    }
}
