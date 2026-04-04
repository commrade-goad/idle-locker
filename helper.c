#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdatomic.h>
#include <time.h>

#include "helper.h"

void run_cmd(const char *cmd) {
    pid_t pid = fork();
    if (pid == 0) {
        execl("/bin/sh", "sh", "-c", cmd, (char *)NULL);
        _exit(127);
    } else if (pid > 0) {
        waitpid(pid, NULL, WNOHANG);
    }
}

void lock_screen(const char *lock_cmd) {
    pid_t pid = fork();
    if (pid < 0) {
        // perror("fork failed");
        return;
    }

    if (pid == 0) {
        // make zombie independent process
        pid_t pid2 = fork();
        if (pid2 == 0) {
            execlp(lock_cmd, lock_cmd, (char *)NULL);
            _exit(127);
        }
        _exit(0);
    }

    waitpid(pid, NULL, 0);
}

bool inhibit_file_exists(const char *filepath) {
    return (access(filepath, F_OK) == 0);
}
