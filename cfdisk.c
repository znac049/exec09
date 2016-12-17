/*
 * Copyright 2009 by Brian Dominy <brian@oddchange.com>
 *
 * This file is part of GCC6809.
 *
 * GCC6809 is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * GCC6809 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GCC6809; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

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
#define CF_ERROR_REG 1
#define CF_FEATURES_REG 1
#define CF_SECTOR_COUNT_REG 2
#define CF_LSN0_REG 3
#define CF_LSN1_REG 4
#define CF_LSN2_REG 5
#define CF_LSN3_REG 6
#define CF_STATUS_REG 7
#define CF_COMMAND_REG 7

#define CF_ 0
#define CF_ 0


/* Emulate an IDE Compact Flash card or cards.
   */

#define MAXSTR 256

#define BYTES_PER_SECTOR 512

void path_init (struct pathlist *path);
void path_add (struct pathlist *path, const char *dir);
FILE * file_open (struct pathlist *path, const char *filename, const char *mode);
FILE * file_require_open (struct pathlist *path, const char *filename, const char *mode);
void file_close (FILE *fp);
char *file_get_fqname();

struct cf_disk
{
  char *name;
  FILE *fd;
  long lsn;
  int count;
  long max_lsn;
};

struct cf_controller
{
  struct cf_disk disk[2];
  int controllerId;
};

static int controllerId = 0;

U8 compact_flash_read (struct hw_device *dev, unsigned long addr)
{
  struct cf_controller *controller = dev->priv;
  struct cf_disk *disk = &controller->disk[0];
  int retval;

  switch (addr) {
  case CF_DATA_REG:
    break;

  case CF_ERROR_REG:
    break;

  case CF_SECTOR_COUNT_REG:
    retval = disk->count;
    break;

  case CF_LSN0_REG:
    retval = (int)(disk->lsn & 0xff);
    break;

  case CF_LSN1_REG:
    retval = (int)((disk->lsn >> 8) & 0xff);
    break;

  case CF_LSN2_REG:
    retval = (int)((disk->lsn >> 16) & 0xff);
    break;

  case CF_LSN3_REG:
    retval = (int)((disk->lsn >> 24) & 0xff);
    break;

  case CF_STATUS_REG:
    break;
  }

  return retval;
}

void compact_flash_write (struct hw_device *dev, unsigned long addr, U8 val)
{
  struct cf_controller *controller = dev->priv;
  struct cf_disk *disk = &controller->disk[0];

  switch (addr) {
  case CF_DATA_REG:
    break;

  case CF_FEATURES_REG:
    break;

  case CF_SECTOR_COUNT_REG:
    disk->count = val;
    break;

  case CF_LSN0_REG:
    disk->lsn = (disk->lsn & 0xffffff00) | (long)val;
    break;

  case CF_LSN1_REG:
    disk->lsn = (disk->lsn & 0xffff00ff) | (long)(val<<8);
    break;

  case CF_LSN2_REG:
    disk->lsn = (disk->lsn & 0xff00ffff) | (long)(val<<16);
    break;

  case CF_LSN3_REG:
    disk->lsn = (disk->lsn & 0x00ffffff) | (long)(val<<24);
    break;

  case CF_COMMAND_REG:
    break;
  }
}

void compact_flash_reset (struct hw_device *dev)
{
  struct cf_controller *cfdev = (struct cf_controller *)dev->priv;

  printf("RESET CF device %d!\n", cfdev->controllerId);
}

struct hw_class compact_flash_class = {
    .name = "compact_flash",
    .readonly = 0,
    .reset = compact_flash_reset,
    .read = compact_flash_read,
    .write = compact_flash_write,
  };

//extern U8 null_read (struct hw_device *dev, unsigned long addr);

void connect_to_disk_file(struct cf_controller *cfdev, int diskNum)
{
  struct cf_disk *disk = &cfdev->disk[diskNum];
  char name[MAXSTR];

  sprintf(name, "cfdisk%d-%d.dsk", cfdev->controllerId, diskNum);

  //printf("DISK: %s\n", name);
  disk->fd = file_open(NULL, name, "r");

  disk->name = strdup(file_get_fqname());
  disk->lsn = disk->max_lsn = 0L;

  if (disk->fd) {
    long endpos;

    // reopen the file for writing
    file_close(disk->fd);
    disk->fd = fopen(disk->name, "a");

    fseek(disk->fd, 0, SEEK_END);
    endpos = ftell(disk->fd);
    disk->max_lsn = endpos / BYTES_PER_SECTOR;
    //printf("MAX LSN = %ld\n", disk->max_lsn);
  }
}

struct hw_device* compact_flash_create (void)
{
  struct cf_controller *cfdev = malloc (sizeof (struct cf_controller));

  connect_to_disk_file(cfdev, 0);
  connect_to_disk_file(cfdev, 1);

  cfdev->controllerId = controllerId;
  controllerId++;

  return device_attach (&compact_flash_class, 4, cfdev);
}
