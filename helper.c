#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>

#include "helper.h"

void run_cmd(const char *cmd) {
    pid_t pid = fork();
    if (pid == 0) {
        execl("/bin/sh", "sh", "-c", cmd, (char *)NULL);
        _exit(127);
    } else if (pid > 0) {
        int status;
        waitpid(pid, &status, WNOHANG);
    }
}

void lock_screen(const char *lock_cmd) {
    pid_t pid = fork();
    if (pid == 0) {
        setsid();
        execlp(lock_cmd, lock_cmd, (char *)NULL);
        _exit(127);
    }
}

void lock_screen_blocking(const char *lock_cmd) {
    pid_t pid = fork();
    if (pid == 0) {
        execlp(lock_cmd, lock_cmd, (char *)NULL);
        _exit(127);
    } else if (pid > 0) {
        int status;
        waitpid(pid, &status, 0); // BLOCKING
    }
}

bool inhibit_file_exists(const char *filepath) {
    return (access(filepath, F_OK) == 0);
}
