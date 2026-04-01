#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdatomic.h>
#include <time.h>

#include "helper.h"

static atomic_bool is_locked = false;
static time_t last_lock_time = 0;

#define LOCK_COOLDOWN 2

void run_cmd(const char *cmd) {
    pid_t pid = fork();
    if (pid == 0) {
        execl("/bin/sh", "sh", "-c", cmd, (char *)NULL);
        _exit(127);
    } else if (pid > 0) {
        waitpid(pid, NULL, WNOHANG);
    }
}

static bool should_lock(void) {
    time_t now = time(NULL);

    if (atomic_exchange(&is_locked, true))
        return false;

    if (now - last_lock_time < LOCK_COOLDOWN) {
        atomic_store(&is_locked, false);
        return false;
    }

    last_lock_time = now;
    return true;
}

void try_lock_screen(const char *lock_cmd) {
    if (!should_lock())
        return;

    pid_t pid = fork();
    if (pid == 0) {
        execlp(lock_cmd, lock_cmd, (char *)NULL);
        _exit(127);
    }

    waitpid(pid, NULL, 0);

    atomic_store(&is_locked, false);
}

bool inhibit_file_exists(const char *filepath) {
    return (access(filepath, F_OK) == 0);
}
