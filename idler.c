#include "idler.h"
#include "helper.h"
#include "config.h"

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <X11/Xlib.h>
#include <dbus/dbus.h>
#include <X11/extensions/scrnsaver.h>
#include <X11/extensions/dpms.h>

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

    DBusError dberr;
    dbus_error_init(&dberr);
    DBusConnection *conn = dbus_bus_get(DBUS_BUS_SYSTEM, &dberr);
    if (!conn) {
        fprintf(stderr, "idle-thread: failed to connect to system bus: %s\n",
                dberr.message ? dberr.message : "(null)");
        if (dbus_error_is_set(&dberr)) dbus_error_free(&dberr);
        return NULL;
    }

    dbus_bus_add_match(conn,
        "type='signal',interface='org.freedesktop.login1.Manager',member='PrepareForSleep'",
        &dberr);
    dbus_connection_flush(conn);
    if (dbus_error_is_set(&dberr)) {
        fprintf(stderr, "idle-thread: dbus match error: %s\n", dberr.message);
        dbus_error_free(&dberr);
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

        dbus_connection_read_write(conn, 1000);
        DBusMessage *msg = dbus_connection_pop_message(conn);
        if (!msg) continue;

        if (dbus_message_is_signal(msg,
                                   "org.freedesktop.login1.Manager",
                                   "PrepareForSleep")) {
            DBusMessageIter args;
            dbus_message_iter_init(msg, &args);

            if (dbus_message_iter_get_arg_type(&args) == DBUS_TYPE_BOOLEAN) {
                dbus_bool_t going_to_sleep = false;
                dbus_message_iter_get_basic(&args, &going_to_sleep);

                if (going_to_sleep) {
                    XForceScreenSaver(dpy, ScreenSaverActive);
                    XFlush(dpy);
                } else {
                    fprintf(stderr, "idle-thread: system woke up\n");
                }
            }
        }

        dbus_message_unref(msg);

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

    bool locked = false;
    XEvent e;
    for (;;) {
        XNextEvent(dpy, &e);

        if (e.type == ev_base + ScreenSaverNotify) {
            XScreenSaverNotifyEvent *xss_ev = (XScreenSaverNotifyEvent *)&e;

            if (xss_ev->state == ScreenSaverOn) {
                if (!locked) {
                    locked = true;
                    fprintf(stderr, "screensaver-thread: X screensaver activated -> locking\n");
                    lock_screen(LOCK_COMMAND);
                } else {
                    fprintf(stderr, "screensaver-thread: Already locked, ignoring event.\n");
                }
            } else if (xss_ev->state == ScreenSaverOff) {
                fprintf(stderr, "screensaver-thread: X screensaver deactivated\n");
                locked = false;
            }
        }
    }

    XCloseDisplay(dpy);
    return NULL;
}
