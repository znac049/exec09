/*
 * Copyright 2009 by Brian Dominy <brian@oddchange.com>
 *
 * This file is part of GCC6809.
[6~ *
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

#define CMD_READLONG_RETRY 0x22
#define CMD_READLONG 0x23
#define CMD_EJECT 0xed
#define CMD_NOP 0x00
#define CMD_SEEK 0x70
#define CMD_WRITELONG_RETRY 0x32
#define CMD_WRITELONG 0x33
#define CMD_DOOR_LOCK 0xde
#define CMD_DOOR_UNLOCK 0xdf
#define CMD_IDENTIFY_DEVICE 0xec
#define CMD_SET_FEATURES 0xef
#define CMD_SET_MULTIPLE_MODE 0xc6
#define CMD_EXECUTE_DEVICE_DIAGNOSTIC 0x90

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
  U8 status_reg;
  U8 error_reg;
  U8 features_reg;
  U8 data[BYTES_PER_SECTOR];
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
    retval = controller->error_reg;
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
    retval = controller->status_reg;
    break;
  }

  return retval;
}

void exec_features_command(struct cf_controller *controller, struct cf_disk *disk, U8 cmd)
{
  switch (cmd) {
  case 0x01: /* Enable 8-bit  data transfers */
    break;

  case 0x02: /* Enable write cache */
    break;

  case 0x03: /* Set transfer mode */
    break;

  case 0x33: /* Disable retry */
    break;

  case 0x44: /* Length of vendor specific bytes on read/write long commands */
    break;

  case 0x54: /* Set cache segments too Sector Count register */
    break;

  case 0x55: /* Disable read lookahead feature */
    break;

  case 0x66: /* Disable reverting to power on defaults */
    break;

  case 0x77: /* Disable ECC */
    break;

  case 0x81: /* Disable 8-bit data transfers */
    break;

  case 0x82: /* Disable write cache */
    break;

  case 0x88: /* Disable CRC */
    break;

  case 0x99: /* Enable retries */
    break;

  case 0xaa: /* Enable read look ahead */
    break;

  case 0xab: /* Set maximum prefetch */
    break;

  case 0xbb: /* 4 bytes of vendor sprecific bytes on read/write long */
    break;

  case 0xcc: /* Enable reverting to power on defaults */
    break;

  default:
    break;
  }
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
    switch(val) {
    case CMD_EXECUTE_DEVICE_DIAGNOSTIC:
      {
	U8 code = 0;

	if (disk->fd) {
	  code = 0x01;
	}
	controller->error_reg = code;
      }
      break;

    case CMD_SET_FEATURES:
      exec_features_command(controller, disk, val);
      break;

    case CMD_WRITELONG:
    case CMD_WRITELONG_RETRY:
      break;

    case CMD_READLONG:
    case CMD_READLONG_RETRY:
      break;
    }
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
