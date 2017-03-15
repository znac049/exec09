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


#define SD_DATA_REG 0
#define SD_CTRL_REG 1
#define SD_STATUS_REG 1
#define SD_LBA0_REG 2
#define SD_LBA1_REG 3
#define SD_LBA2_REG 4
#define SD_LBA3_REG 5

#define SD_READ_CMD 0

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
  long lba;
  long max_lba;
  int index;
  U8 status;
  U8 data[BYTES_PER_LBA];
};

static struct pathlist sd_path;
static int controllerId = 0;

U8 sdcard_read(struct hw_device *dev, unsigned long addr)
{
  struct sd_controller *sddev = dev->priv;
  int retval=0;

  switch (addr) {
  case SD_DATA_REG:
    if (sddev->index == -1) {
      debugf(10, "Attempt to read SD data when there isn't any!");
    }
    else {
      retval = sddev->data[sddev->index];
      sddev->index++;
      if (sddev->index >= BYTES_PER_LBA) {
	sddev->index = -1;
	sddev->status &= 0x9f;
      }
    }
    break;

  case SD_STATUS_REG:
    // we are always ready!
    retval = sddev->status;
    break;

  default:
    debugf(20, "Read UNKNOWN Reg: 0x%02x\n", retval);
    break;
  }

  return retval;
}

void read_lba(struct sd_controller *sddev)
{
  int numBytes;
  long seekPos;

  debugf(10, "SD read LBA %ld\n", sddev->lba);

  sddev->index = -1;

  if (sddev->lba > sddev->max_lba) {
    debugf(10, "Attempt to read LBA %ld which is past the end of the device (%ld)\n", sddev->lba, sddev->max_lba);
    return;
  }

  seekPos = sddev->lba * BYTES_PER_LBA;
  debugf(10, "Seeking to offset %ld\n", seekPos);

  if (fseek(sddev->fd, seekPos, SEEK_SET) == -1) {
    debugf(10, "Seek failed on LBA %ld\n", sddev->lba);
    return;
  }

  numBytes = fread(sddev->data, 1, BYTES_PER_LBA, sddev->fd);
  debugf(10, "Read %d bytes\n", numBytes);

  if (numBytes != BYTES_PER_LBA) {
    debugf(10, "Truncated read on LBA %ld\n", sddev->lba);
    return;
  }

  debugf(10, "SD Read LBA %ld success\n", sddev->lba);

  sddev->index = 0;
  sddev->status |= 0x60;
}

void sdcard_write (struct hw_device *dev, unsigned long addr, U8 val)
{
  struct sd_controller *sddev = dev->priv;

  switch (addr) {
  case SD_CTRL_REG:
    switch (val) {
    case SD_READ_CMD:
      read_lba(sddev);
      break;
    }
    break;

  case SD_LBA0_REG:
    {
      long lba = sddev->lba & 0xfff0;
      
      sddev->lba = lba | (((long)val) & 0x000f);
    }
    break;

  case SD_LBA1_REG:
    {
      long lba = sddev->lba & 0xff0f;
      
      sddev->lba = lba | ((((long)val) & 0x000f) << 8);
    }
    break;

  case SD_LBA2_REG:
    {
      long lba = sddev->lba & 0xf0ff;
      
      sddev->lba = lba | ((((long)val) & 0x000f) << 16);
    }
    break;

  case SD_LBA3_REG:
    {
      long lba = sddev->lba & 0x0fff;
      
      sddev->lba = lba | ((((long)val) & 0x000f) << 24);
    }
    break;

  default:
    debugf(20, "SD_write() bad addr=$%04lx, val=$%02x\n", addr, val);
    break;
  }
}

void sdcard_reset (struct hw_device *dev)
{
  struct sd_controller *sddev = (struct sd_controller *)dev->priv;

  sddev->status = 0x80;
  sddev->lba = 0;
  sddev->index = -1;

  printf("RESET SD device %d!\n", sddev->controllerId);
}

struct hw_class sdcard_class = {
    .name = "SD card",
    .readonly = 0,
    .reset = sdcard_reset,
    .read = sdcard_read,
    .write = sdcard_write,
  };

//extern U8 null_read (struct hw_device *dev, unsigned long addr);

void connect_to_sdcard_file(struct sd_controller *sddev)
{
  char name[MAXSTR];

  sprintf(name, "sdcard%d.dsk", sddev->controllerId);

  printf("SDCARD: %s\n", name);
  sddev->fd = file_open(&sd_path, name, "r");

  sddev->name = strdup(file_get_fqname());
  sddev->lba = sddev->max_lba = 0L;

  if (sddev->fd) {
    long endpos;

    // reopen the file for writing
    file_close(sddev->fd);
    debugf(10, "Reopen %s for write.\n", sddev->name);
    sddev->fd = fopen(sddev->name, "r+");
    fseek(sddev->fd, 0, SEEK_END);
    endpos = ftell(sddev->fd);
    sddev->max_lba = endpos / BYTES_PER_LBA;
    printf("SD Card MAX LBA = %ld\n", sddev->max_lba);
  }
}

struct hw_device* sdcard_create (void)
{
  struct sd_controller *sddev = malloc (sizeof (struct sd_controller));

  if (controllerId == 0) {
    char *sd = getenv("SDDIR");
    
    path_init(&sd_path);

    if (sd != NULL) {
      path_add(&sd_path, sd);
    }

    path_add(&sd_path, "Images");
  }

  sddev->controllerId = controllerId;
  connect_to_sdcard_file(sddev);

  controllerId++;

  return device_attach (&sdcard_class, 4, sddev);
}
