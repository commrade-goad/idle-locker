#include "idler.h"
#include <pthread.h>

int main(void) {
    pthread_t idle_t, logind_t, screensaver_t;

    pthread_create(&idle_t, NULL, idle_thread, NULL);
    pthread_create(&logind_t, NULL, logind_thread, NULL);
    pthread_create(&screensaver_t, NULL, screensaver_thread, NULL);

    pthread_join(idle_t, NULL);
    pthread_join(logind_t, NULL);
    pthread_join(screensaver_t, NULL);

    return 0;
}
