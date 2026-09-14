#include <dirent.h>
#include <errno.h>
#include <locale.h>
#include <ncurses.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STB_IMAGE_IMPLEMENTATION
#include "../thirdparty/stb_image.h"
#define ICAT_IMPLEMENTATION
#include "icat.h"
#define NITROGEN_IMPLEMENTATION
#include "nitrogen.h"

struct view {
  WINDOW *win;
  int height;
  int width;
};
typedef struct view View;

struct file_entry {
  char *name;
  int type;
};

struct file_entrys {
  struct file_entry *data;
  size_t size;
  size_t selected;
  char *dir;
};
typedef struct file_entrys File_Entrys;

int init_ncurses();

View *create_view(int height, int width, int starty, int startx);
void create_views(View **file_view, View **image_view);
void clear_view(View *view);

void load_files(View *file_view, File_Entrys *entrys, char *target_dir);
void list_files(View *file_view, File_Entrys *entrys);

void print_image(View *image_view, char *dir, char *name);
void set_image(char *dir, char *name);

void select_entry(View *file_view, View *image_view, File_Entrys *entrys,
                  size_t prev_entry);
void open_entry(View *file_view, View *image_view, File_Entrys *entrys);
void free_entrys(File_Entrys *entrys);

const wchar_t *FOLDER_ICON = L"";
const wchar_t *IMAGE_ICON = L"󰋩";

int main(void) {
  View *file_view = NULL, *image_view = NULL;

  char *target_dir = strdup("/home/ben/Pictures/wallpapers/");
  File_Entrys *entrys = calloc(1, sizeof(File_Entrys));

  if (init_ncurses() > 0)
    return 1;

  // TODO: show keybinds and options
  create_views(&file_view, &image_view);
  load_files(file_view, entrys, target_dir);
  list_files(file_view, entrys);

  wmove(file_view->win, 1, 1);
  wrefresh(file_view->win);

  int ch = 0;
  while (ch != 'q') {
    ch = getch();
    // TODO: check for resize

    if (ch == 'k' || ch == KEY_UP) {
      size_t prev = entrys->selected;
      if (prev == 0) {
        entrys->selected = entrys->size - 1;
      } else {
        entrys->selected--;
      }
      select_entry(file_view, image_view, entrys, prev);
    } else if (ch == 'j' || ch == KEY_DOWN) {
      size_t prev = entrys->selected;
      if (prev + 2 > entrys->size) {
        entrys->selected = 0;
      } else {
        entrys->selected++;
      }

      select_entry(file_view, image_view, entrys, prev);
    } else if (ch == 'o' || ch == '\n') {
      open_entry(file_view, image_view, entrys);
    } else if (ch == 's') {
      set_image(entrys->dir, entrys->data[entrys->selected].name);
    }
  }

  endwin();
  return 0;
}

void open_entry(View *file_view, View *image_view, File_Entrys *entrys) {
  if (entrys->data[entrys->selected].type == DT_DIR) {
    char *name = entrys->data[entrys->selected].name;
    size_t path_len = strlen(entrys->dir) + strlen(name) + 2; // slash + '\0'
    char *path = malloc(path_len);
    if (!path)
      return;

    snprintf(path, path_len, "%s%s/", entrys->dir, name);

    clear_view(file_view);
    load_files(file_view, entrys, path);
    list_files(file_view, entrys);
  } else {
    clear_view(image_view);
    print_image(image_view, entrys->dir, entrys->data[entrys->selected].name);
  }
}

void set_image(char *dir, char *name) {
  char *path = malloc(strlen(dir) + strlen(name) + 1);
  strcpy(path, dir);
  strcat(path, name);

  // TODO: get options from user
  nitrogen_set_wallpaper(path, MONITOR_EACH, AUTO);

  free(path);
}

void print_image(View *image_view, char *dir, char *name) {
  wmove(image_view->win, 1, 1);
  wrefresh(image_view->win);

  char *path = malloc(strlen(dir) + strlen(name) + 1);
  strcpy(path, dir);
  strcat(path, name);

  int status = icat_print_image_rect(path, 3, image_view->width - 2, -1);
  if (status != 0) {
    wprintw(image_view->win, "image not found: %s", path);
    wrefresh(image_view->win);
  }
  free(path);
  wrefresh(image_view->win);
}

void print_entry(View *file_view, size_t index, File_Entrys *entrys) {
  wchar_t *icon = (wchar_t *)IMAGE_ICON;
  if (entrys->data[index].type == DT_DIR) {
    icon = (wchar_t *)FOLDER_ICON;
  }

  mvwprintw(file_view->win, index + 1, 1, " %ls %s", icon,
            entrys->data[index].name);

  if (entrys->selected == index) {
    mvwchgat(file_view->win, index + 1, 1, file_view->width - 2, COLOR_PAIR(2),
             1, NULL);
  }
}

void select_entry(View *file_view, View *image_view, File_Entrys *entrys,
                  size_t prev_entry) {
  mvwchgat(file_view->win, prev_entry + 1, 1, file_view->width - 2,
           COLOR_PAIR(2), 0, NULL);
  mvwchgat(file_view->win, entrys->selected + 1, 1, file_view->width - 2,
           COLOR_PAIR(2), 1, NULL);
  wrefresh(file_view->win);

  /* TODO: show only after delay
  struct file_entry entry = entrys->data[entrys->selected];
  if (entry.type == DT_DIR)
    return;
  print_image(image_view, entrys->dir, entry.name);
  */
}

int entryCompare(const void *a, const void *b) {
  const struct file_entry *ea = (const struct file_entry *)a;
  const struct file_entry *eb = (const struct file_entry *)b;

  if (ea->name == NULL && eb->name == NULL)
    return 0;
  if (ea->name == NULL)
    return -1;
  if (eb->name == NULL)
    return 1;

  return strcmp(ea->name, eb->name);
}

void load_files(View *file_view, File_Entrys *entrys, char *path) {
  static DIR *FD;
  static struct dirent *in_file;

  free(entrys->dir);
  entrys->dir = path;
  free_entrys(entrys);

  FD = opendir(path);
  if (FD == NULL) {
    wprintw(file_view->win, "Error : Failed to open target directory - %s\n",
            strerror(errno));
    return;
  }

  size_t entrys_size = 0;
  while (readdir(FD) != NULL) {
    entrys_size++;
  }

  // skip "." dir
  entrys_size--;

  rewinddir(FD);

  struct file_entry *current_entrys =
      calloc(entrys_size, sizeof(struct file_entry));

  size_t i = 0;
  while ((in_file = readdir(FD)) != NULL && i < entrys_size) {
    if (strcmp(in_file->d_name, ".") == 0)
      continue;
    current_entrys[i].name = strdup(in_file->d_name);
    current_entrys[i].type = in_file->d_type;
    i++;
  }

  qsort(current_entrys, entrys_size, sizeof(struct file_entry), entryCompare);

  closedir(FD);

  entrys->data = current_entrys;
  entrys->size = entrys_size;
}

void list_files(View *file_view, File_Entrys *entrys) {
  for (size_t i = 0; i < entrys->size; i++) {
    print_entry(file_view, i, entrys);
  }

  wrefresh(file_view->win);
}

void free_entrys(File_Entrys *entrys) {

  for (size_t i = 0; i < entrys->size; i++) {
    free(entrys->data[i].name);
  }
  free(entrys->data);
  entrys->selected = 0;
  entrys->size = 0;
}

void create_views(View **file_view, View **image_view) {
  int height = LINES;
  int file_width = COLS / 4;
  int image_width = COLS - file_width;
  refresh();

  *file_view = create_view(height, file_width, 0, 0);
  *image_view = create_view(height, image_width, 0, file_width);

  wmove((*file_view)->win, 1, 1);
  wrefresh((*file_view)->win);
  wmove((*image_view)->win, 1, 1);
  wrefresh((*image_view)->win);
}

View *create_view(int height, int width, int starty, int startx) {
  View *view = malloc(sizeof *view);
  if (!view)
    return NULL;

  WINDOW *win = newwin(height, width, starty, startx);
  box(win, 0, 0);

  view->win = win;
  view->width = width;
  view->height = height;

  return view;
}

void clear_view(View *view) {
  wclear(view->win);
  box(view->win, 0, 0);
}

int init_ncurses() {
  setlocale(LC_ALL, "");
  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  raw();
  curs_set(0);

  if (has_colors() == FALSE) {
    endwin();
    printf("Your terminal does not support color\n");
    return 1;
  }
  start_color();
  use_default_colors();

  init_color(COLOR_WHITE, 1000, 1000, 1000);
  init_pair(2, COLOR_BLUE, COLOR_WHITE);
  init_pair(1, COLOR_WHITE, COLOR_BLUE);

  return 0;
}
