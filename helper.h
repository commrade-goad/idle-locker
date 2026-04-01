#ifndef HELPER_H
#define HELPER_H

void run_cmd(const char *cmd);
void try_lock_screen(const char *lock_cmd);
bool inhibit_file_exists(const char *filepath);

#endif /* HELPER_H */
