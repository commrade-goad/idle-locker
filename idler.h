#ifndef IDLER_H
#define IDLER_H

void *idle_thread(void *arg);
void *logind_thread(void *arg);
void *screensaver_thread(void *arg);

#endif /* IDLER_H */
