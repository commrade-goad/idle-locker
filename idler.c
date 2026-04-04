#include "idler.h"
#include "helper.h"
#include "config.h"

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdatomic.h>
#include <sys/types.h>
#include <X11/Xlib.h>
#include <X11/extensions/scrnsaver.h>
#include <X11/extensions/dpms.h>

static atomic_bool is_locked = false;

void *idle_thread(void *arg) {
    (void) arg;
    Display *dpy = XOpenDisplay(NULL);
    if (!dpy) {
        fprintf(stderr, "idle-thread: cannot open display\n");
        return NULL;
    }

    int ev, err;
    if (!XScreenSaverQueryExtension(dpy, &ev, &err)) {
        fprintf(stderr, "idle-thread: no XScreenSaver extension\n");
        XCloseDisplay(dpy);
        return NULL;
    }

    XScreenSaverInfo *info = XScreenSaverAllocInfo();
    bool changed = false;

    XSetScreenSaver(dpy, SCREENSAVER_TIMEOUT, SCREENSAVER_TIMEOUT,
            DefaultBlanking, DefaultExposures);
    DPMSEnable(dpy);
    DPMSSetTimeouts(dpy, DPMS_TIMEOUT, DPMS_TIMEOUT, DPMS_TIMEOUT);

    for (;;) {
        XScreenSaverQueryInfo(dpy, DefaultRootWindow(dpy), info);
        unsigned long idle = info->idle;
        bool inhibited = inhibit_file_exists(INHIBIT_FILE);

        if (inhibited && !changed) {
            XSetScreenSaver(dpy, 0, 0, DefaultBlanking, DefaultExposures);
            DPMSDisable(dpy);
            changed = true;
        } else if (!inhibited && idle >= SUSPEND_THRESHOLD) {
            DPMSForceLevel(dpy, DPMSModeOff);
            sleep(1);
            run_cmd(SUSPEND_COMMAND);
        } else if (!inhibited && changed) {
            XSetScreenSaver(dpy, SCREENSAVER_TIMEOUT, SCREENSAVER_TIMEOUT,
                    DefaultBlanking, DefaultExposures);
            DPMSEnable(dpy);
            DPMSSetTimeouts(dpy, DPMS_TIMEOUT, DPMS_TIMEOUT, DPMS_TIMEOUT);
            changed = false;
        }

        sleep(IDLE_MANAGER_INTERVAL);
    }

    XFree(info);
    XCloseDisplay(dpy);
    return NULL;
}

void *screensaver_thread(void *arg) {
    (void) arg;
    Display *dpy = XOpenDisplay(NULL);
    if (!dpy) {
        fprintf(stderr, "screensaver-thread: cannot open display\n");
        return NULL;
    }

    int ev_base, err_base;
    if (!XScreenSaverQueryExtension(dpy, &ev_base, &err_base)) {
        fprintf(stderr, "screensaver-thread: no XScreenSaver extension\n");
        XCloseDisplay(dpy);
        return NULL;
    }

    XScreenSaverSelectInput(dpy, DefaultRootWindow(dpy), ScreenSaverNotifyMask);

    XEvent e;
    for (;;) {
        XNextEvent(dpy, &e);

        if (e.type == ev_base + ScreenSaverNotify) {
            XScreenSaverNotifyEvent *xss_ev = (XScreenSaverNotifyEvent *)&e;

            if (xss_ev->state == ScreenSaverOn) {
                // Try to acquire the lock. If it was false, we set it to true and proceed.
                if (!atomic_exchange(&is_locked, true)) {
                    fprintf(stderr, "screensaver-thread: X screensaver activated -> locking\n");
                    lock_screen(LOCK_COMMAND);
                } else {
                    fprintf(stderr, "screensaver-thread: Already locked, ignoring event.\n");
                }
            } else if (xss_ev->state == ScreenSaverOff) {
                fprintf(stderr, "screensaver-thread: X screensaver deactivated\n");

                // User unlocked the screen! Reset the flag so it can lock again next time.
                atomic_store(&is_locked, false);
            }
        }
    }

    XCloseDisplay(dpy);
    return NULL;
}
