#ifndef COMMAND_H
#define COMMAND_H

void ls(const char *path);
void cd(const char *path);
void pwd();
void mkdir(const char *path);
void rmdir(const char *path);
void touch(const char *path);
void rm(const char *path);
void cp(const char *args);
void mv(const char *args);
void execute_command(const char *command, const char *args);

#endif
