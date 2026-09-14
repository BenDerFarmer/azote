#ifndef NITROGEN_H
#define NITROGEN_H

#include <X11/Xlib.h>
#include <X11/extensions/Xinerama.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

enum Display_Option { AUTO, CENTERED, SCALED, TILED, ZOOM, ZOOM_FILL };

#define MONITOR_FULL -1
#define MONITOR_EACH -2

int nitrogen_set_wallpaper(char *path, int monitor, enum Display_Option option);

#ifdef NITROGEN_IMPLEMENTATION

char *option_to_string(enum Display_Option option) {
  switch (option) {
  case 0:
    return "--set-auto";
  case 1:
    return "--set-centered";
  case 2:
    return "--set-scaled";
  case 3:
    return "--set-tiled";
  case 4:
    return "--set-zoom";
  case 5:
    return "--set-zoom-fill";
  default:
    return NULL;
  }
}

int get_screen_count() {
  Display *dpy = XOpenDisplay(NULL);
  if (!dpy) {
    fprintf(stderr, "Failed to open display\n");
    return 1;
  }

  int event_base, error_base;
  if (!XineramaQueryExtension(dpy, &event_base, &error_base)) {
    fprintf(stderr, "Xinerama not supported\n");
    XCloseDisplay(dpy);
    return 1;
  }

  if (!XineramaIsActive(dpy)) {
    fprintf(stderr, "Xinerama is not active\n");
    XCloseDisplay(dpy);
    return 1;
  }

  int nscreens = 0;
  XineramaScreenInfo *screens = XineramaQueryScreens(dpy, &nscreens);
  if (!screens) {
    fprintf(stderr, "XineramaQueryScreens failed\n");
    XCloseDisplay(dpy);
    return 1;
  }

  return nscreens;
}

#define NITROGEN_BASE_COMMAND "/bin/nitrogen"
#define NITROGEN_SET_MONITOR "--head="

extern char **environ;

int nitrogen_set_wallpaper(char *path, int monitor,
                           enum Display_Option option) {
  char *option_string = option_to_string(option);
  size_t monitor_string_len = strlen(NITROGEN_SET_MONITOR) + 11 + 1;
  char *monitor_string = (char *)malloc(monitor_string_len);

  pid_t pid = fork();
  if (pid < 0) {
    free(monitor_string);
    return -1;
  }

  if (pid == 0) {
    int devnull = open("/dev/null", O_WRONLY);
    if (devnull >= 0) {
      dup2(devnull, STDOUT_FILENO);
      dup2(devnull, STDERR_FILENO);
      close(devnull);
    }

    int count = (monitor == MONITOR_EACH) ? get_screen_count() : 1;

    for (int i = 0; i < count; i++) {
      snprintf(monitor_string, monitor_string_len, "%s%d", NITROGEN_SET_MONITOR,
               i);

      pid_t c = fork();
      if (c == 0) {
        execlp(NITROGEN_BASE_COMMAND, NITROGEN_BASE_COMMAND, option_string,
               path, monitor_string, (char *)NULL);
        _exit(127);
      }
    }

    _exit(0);
  }

  free(monitor_string);
  return 0;
}

#endif // NITROGEN_IMPLEMENTATION

#endif // !NITROGEN_H
