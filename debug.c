#include <stdio.h>
#include <stdarg.h>

static FILE *dbg = NULL;
static int dbg_level = 1;

void set_debug(int level, const char *file)
{
  if (file) {
    if (dbg) {
      fclose(dbg);
    }

    dbg = fopen(file, "a");
  }

  dbg_level = level;
}

void debugf(int level, const char *fmt, ...)
{
  va_list args;

  if (level > dbg_level) {
    return;
  }

  if (dbg) {
    va_start(args, fmt);
    vfprintf(dbg, fmt, args);
    fflush(dbg);
    va_end(args);
  }
}
