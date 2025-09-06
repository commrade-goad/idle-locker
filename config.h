#ifndef CONFIG_H
#define CONFIG_H

#define SCREENSAVER_TIMEOUT 300              // seconds
#define DPMS_TIMEOUT        300              // seconds
#define SUSPEND_THRESHOLD   (10 * 60 * 1000) // 10 minutes

#define LOCK_COMMAND "slock"
#define INHIBIT_FILE "/tmp/idle_inhibit"
#define SUSPEND_COMMAND "systemctl suspend"

#define IDLE_MANAGER_INTERVAL 5

#endif /* CONFIG_H */
