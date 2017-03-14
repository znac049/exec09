#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "6809.h"
#include "machine.h"


#define CF_DATA_REG 0


/* Emulate an SD card interface
   */

#define MAXSTR 256

#define BYTES_PER_LBA 512

void path_init (struct pathlist *path);
void path_add (struct pathlist *path, const char *dir);
FILE *file_open (struct pathlist *path, const char *filename, const char *mode);
FILE *file_require_open (struct pathlist *path, const char *filename, const char *mode);
void file_close (FILE *fp);
char *file_get_fqname();

struct sd_controller
{
  int controllerId;
  short int disk_id;

  char *name;
  FILE *fd;
  long lsn;
  int count;
  int index;
  long max_lsn;
  U8 data[BYTES_PER_LBA];
};

static struct pathlist sd_path;
static int controllerId = 0;

U8 sdcard_read (struct hw_device *dev, unsigned long addr)
{
  struct sd_controller *sddev = dev->priv;
  int retval=0;

  switch (addr) {
  default:
    debugf(20, "Read UNKNOWN Reg: 0x%02x\n", retval);
    break;
  }

  return retval;
}

void sdcard_write (struct hw_device *dev, unsigned long addr, U8 val)
{
  struct sd_controller *sddev = dev->priv;

  switch (addr) {
  default:
    debugf(20, "SD_write() bad addr=$%04lx, val=$%02x\n", addr, val);
    break;
  }
}

void sdcard_reset (struct hw_device *dev)
{
  struct sd_controller *sddev = (struct sd_controller *)dev->priv;

  printf("RESET CF device %d!\n", sddev->controllerId);
}

struct hw_class sdcard_class = {
    .name = "compact_flash",
    .readonly = 0,
    .reset = sdcard_reset,
    .read = sdcard_read,
    .write = sdcard_write,
  };

//extern U8 null_read (struct hw_device *dev, unsigned long addr);

void connect_to_sdcard_file(struct sd_controller *sddev, int diskNum)
{
  char name[MAXSTR];

  sprintf(name, "sdcard%d-%d.dsk", sddev->controllerId, diskNum);

  //printf("DISK: %s\n", name);
  sddev->fd = file_open(&sd_path, name, "r");

  sddev->name = strdup(file_get_fqname());
  sddev->lsn = sddev->max_lsn = 0L;
  sddev->count = sddev->index = 0;

  if (sddev->fd) {
    long endpos;

    // reopen the file for writing
    file_close(sddev->fd);
    sddev->fd = fopen(sddev->name, "a");

    fseek(sddev->fd, 0, SEEK_END);
    endpos = ftell(sddev->fd);
    sddev->max_lsn = endpos / BYTES_PER_LBA;
    //printf("MAX LSN = %ld\n", sddev->max_lsn);
  }
}

struct hw_device* sdcard_create (void)
{
  struct sd_controller *sddev = malloc (sizeof (struct sd_controller));

  if (controllerId == 0) {
    char *cf = getenv("SDDIR");
    
    path_init(&sd_path);

    if (cf != NULL) {
      path_add(&sd_path, cf);
    }

    path_add(&sd_path, "Images");
  }

  connect_to_sdcard_file(sddev, 0);

  sddev->controllerId = controllerId;
  controllerId++;

  return device_attach (&sdcard_class, 4, sddev);
}
