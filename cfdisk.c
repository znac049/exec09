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
#define CMD_WRITELONG_RETRY 0x32
#define CMD_WRITELONG 0x33
#define CMD_EXECUTE_DEVICE_DIAGNOSTIC 0x90
#define CMD_INIT_DEVICE_PARAMS 0x91
#define CMD_IDENTIFY_DEVICE 0xec
#define CMD_SET_FEATURES 0xef

#define SR_BUSY 0x80
#define SR_DRDY 0x40
#define SR_DF   0x20
#define SR_DSC  0x10
#define SR_DRQ  0x08
#define SR_CORR 0x04
#define SR_IDX  0x02
#define SR_ERR  0x01

#define ER_UNC  0x40
#define ER_MC   0x20
#define ER_IDNF 0x10
#define ER_MCR  0x08
#define ER_ABRT 0x04
#define ER_TKNONF 0x02
#define ER_AMNF 0x01

#define IS_DRDY(c) (c->status_reg & SR_DRDY)

/* Emulate an IDE Compact Flash card or cards.
   */

#define MAXSTR 256

#define BYTES_PER_SECTOR 512

void path_init (struct pathlist *path);
void path_add (struct pathlist *path, const char *dir);
FILE *file_open (struct pathlist *path, const char *filename, const char *mode);
FILE *file_require_open (struct pathlist *path, const char *filename, const char *mode);
void file_close (FILE *fp);
char *file_get_fqname();

struct cf_controller
{
  int controllerId;
  U8 status_reg;
  U8 error_reg;
  U8 features_reg;
  U8 eight_bit_mode;
  U8 sectors_per_track;
  U8 num_heads;
  U16 num_cylinders;
  short int disk_id;

  char *name;
  FILE *fd;
  long lsn;
  int count;
  int index;
  long max_lsn;
  U8 data[BYTES_PER_SECTOR];
};

static struct pathlist cf_path;
static int controllerId = 0;

void cf_status(struct cf_controller *cfdev) {
  debugf(20, "ID=%d: LSN: %ld, INDEX:COUNT: %d:%d, MAXLSN: %ld\n", cfdev->disk_id, cfdev->lsn, cfdev->index, cfdev->count, cfdev->max_lsn);
  debugf(20, "SR=0x%02x, ER=0x%02x, FR=0x%02x\n\n", cfdev->status_reg, cfdev->error_reg, cfdev->features_reg);
}

void single_byte_result(U8 val, struct cf_controller *cfdev)
{
  cfdev->data[0] = val;
  cfdev->count = 1;
  cfdev->index = 0;
}

U8 compact_flash_read (struct hw_device *dev, unsigned long addr)
{
  struct cf_controller *cfdev = dev->priv;
  int retval=0;

  switch (addr) {
  case CF_DATA_REG:
    if (cfdev->count) {
      cfdev->count--;
      retval = cfdev->data[cfdev->index];
      cfdev->index++;
      if (cfdev->count == 0) {
	cfdev->status_reg &= ~SR_DRQ;
      }
      debugf(20, "Read DATA Reg: 0x%02x\n", retval);
    }
    else {
      retval = 0;
      debugf(20, "Read DATA Reg underrun!\n");
    }
    break;

  case CF_ERROR_REG:
    retval = cfdev->error_reg;
    debugf(20, "Read CF ERROR Reg: 0x%02x\n", retval);
    break;

  case CF_SECTOR_COUNT_REG:
    retval = cfdev->count;
    debugf(20, "Read SECTOR COUNT Reg: 0x%02x\n", retval);
    break;

  case CF_LSN0_REG:
    retval = (int)(cfdev->lsn & 0xff);
    debugf(20, "Read LSN0 Reg: 0x%02x\n", retval);
    break;

  case CF_LSN1_REG:
    retval = (int)((cfdev->lsn >> 8) & 0xff);
    debugf(20, "Read LSN1 Reg: 0x%02x\n", retval);
    break;

  case CF_LSN2_REG:
    retval = (int)((cfdev->lsn >> 16) & 0xff);
    debugf(20, "Read LSN2 Reg: 0x%02x\n", retval);
    break;

  case CF_LSN3_REG:
    retval = (int)(((cfdev->lsn >> 24) & 0xff) | 0xe0000000);
    debugf(20, "Read LSN3 Reg: 0x%02x\n", retval);
    break;

  case CF_STATUS_REG:
    retval = cfdev->status_reg;
    debugf(20, "Read STATUS Reg: 0x%02x\n", retval);
    break;

  default:
    debugf(20, "Read UNKNOWN Reg: 0x%02x\n", retval);
    break;
  }

  return retval;
}

void set_device_info(struct cf_controller *cfdev)
{
  U16 *words = (U16 *)cfdev->data;
 
  bzero(cfdev->data, BYTES_PER_SECTOR);

  words[0] = 0x0080; // Removable media
  words[1] = cfdev->num_cylinders;
  words[3] = cfdev->num_heads;
  words[6] = cfdev->sectors_per_track;

  strcpy((char *)&words[10], "00000000000000009942");

  words[22] = 0;

  strcpy((char *)&words[23], "FW:0001a");
  strcpy((char *)&words[27], "CF-Bob MK I");

  words[49] = 99;

  cfdev->count = 256;
  cfdev->index = 0;
}

void exec_set_features_cmd(struct cf_controller *cfdev)
{
  debugf(20, "Execute SET_FEATURES command. Feature=0x%02x\n", cfdev->features_reg);

  if (!cfdev->status_reg & SR_DRDY) {
    debugf(20, "Device has not been initialised - aborting command\n");
    
    cfdev->status_reg |= SR_ERR;
    cfdev->error_reg |= ER_ABRT;
    return;
  }

  switch (cfdev->features_reg) {
  case 0x01: /* Enable 8-bit  data transfers */
    cfdev->eight_bit_mode = 1;
    debugf(20, "Enable 8-bit transfers\n");
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
    cfdev->eight_bit_mode = 0;
    debugf(20, "Disable 8-bit transfers\n");
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

void exec_device_diag_cmd(struct cf_controller *cfdev)
{
  U8 code = 0x80;

  debugf(20, "Execute DEVICE DIAG command.\n");

  if (cfdev->fd) {
    debugf(20, "Device 0 found\n");
    code = 0x01;
    cfdev->status_reg |= SR_DRDY;
  }

  cfdev->error_reg = code;
}

void exec_device_params_cmd(struct cf_controller *cfdev)
{
  U8 code = 0;
  int spt = cfdev->count;
  int heads = (cfdev->lsn & 0x000000ff) + 1;
  int cyls = cfdev->num_cylinders;

  debugf(20, "Execute INIT DEVICE PARAMS command.\n");

  debugf(20, "New CHS=%d:%d:%d\n", cyls, heads, spt);

  if (! (cfdev->num_heads == heads) && (cfdev->sectors_per_track == spt)) {
    debugf(20, "CHS not supported\n");

    cfdev->status_reg |= SR_ERR;
    cfdev->error_reg |= ER_ABRT;
  }
}

void exec_identify_cmd(struct cf_controller *cfdev)
{
  debugf(20, "Execute IDENTIFY DEVICE command.\n");

  if (!cfdev->status_reg & SR_DRDY) {
    debugf(20, "Device has not been initialised - aborting command\n");
    
    cfdev->status_reg |= SR_ERR;
    cfdev->error_reg |= ER_ABRT;
    return;
  }

  set_device_info(cfdev);
  cfdev->status_reg |= SR_DRQ;
}

void exec_write_cmd(struct cf_controller *cfdev)
{
  debugf(20, "Execute WRITE LONG command.\n");

  if (!cfdev->status_reg & SR_DRDY) {
    debugf(20, "Device has not been initialised - aborting command\n");
    
    cfdev->status_reg |= SR_ERR;
    cfdev->error_reg |= ER_ABRT;
    return;
  }

}

void exec_read_cmd(struct cf_controller *cfdev)
{
  debugf(20, "Execute WRITE LONG command.\n");

  if (!cfdev->status_reg & SR_DRDY) {
    debugf(20, "Device has not been initialised - aborting command\n");
    
    cfdev->status_reg |= SR_ERR;
    cfdev->error_reg |= ER_ABRT;
    return;
  }

}

void execute_command(U8 cmd, struct cf_controller *cfdev)
{
  cfdev->error_reg = 0;
  cfdev->status_reg &= ~SR_ERR;

  switch(cmd) {
  case CMD_IDENTIFY_DEVICE:
    exec_identify_cmd(cfdev);
    break;

  case CMD_INIT_DEVICE_PARAMS:
    exec_device_params_cmd(cfdev);
    break;

  case CMD_EXECUTE_DEVICE_DIAGNOSTIC:
    exec_device_diag_cmd(cfdev);
    break;

  case CMD_SET_FEATURES:
    exec_set_features_cmd(cfdev);
    break;

  case CMD_WRITELONG:
  case CMD_WRITELONG_RETRY:
    exec_write_cmd(cfdev);
    break;

  case CMD_READLONG:
  case CMD_READLONG_RETRY:
    exec_read_cmd(cfdev);
    break;
  }
}

void compact_flash_write (struct hw_device *dev, unsigned long addr, U8 val)
{
  struct cf_controller *cfdev = dev->priv;

  switch (addr) {
  case CF_DATA_REG:
    debugf(20, "Write DATA REG=0x%02x\n", val);
    break;

  case CF_FEATURES_REG:
    debugf(20, "Write FEATURES REG=0x%02x\n", val);
    cfdev->features_reg = val;
    break;

  case CF_SECTOR_COUNT_REG:
    debugf(20, "Write DATA COUNT=0x%02x\n", val);
    cfdev->count = val;
    break;

  case CF_LSN0_REG:
    debugf(20, "Write LSN0=0x%02x\n", val);
    cfdev->lsn = (cfdev->lsn & 0xffffff00) | (long)val;
    break;

  case CF_LSN1_REG:
    debugf(20, "Write LSN1=0x%02x\n", val);
    cfdev->lsn = (cfdev->lsn & 0xffff00ff) | (long)(val<<8);
    break;

  case CF_LSN2_REG:
    debugf(20, "Write LSN2=0x%02x\n", val);
    cfdev->lsn = (cfdev->lsn & 0xff00ffff) | (long)((val & 0x0f)<<16) & 0x0f;
    cfdev->disk_id = val>>4 & 0x10;
    break;

  case CF_LSN3_REG:
    debugf(20, "Write LSN3=0x%02x\n", val);
    cfdev->disk_id = (val>>4 & 0x01);
    cfdev->lsn = (cfdev->lsn & 0x00ffffff) | (long)((val & 0x0f) <<24);
    break;

  case CF_COMMAND_REG:
    execute_command(val, cfdev);
    break;

  default:
    debugf(20, "CF_write() bad addr=$%04lx, val=$%02x\n", addr, val);
    break;
  }

  cf_status(cfdev);
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
  char name[MAXSTR];

  sprintf(name, "cfdisk%d-%d.dsk", cfdev->controllerId, diskNum);

  //printf("DISK: %s\n", name);
  cfdev->fd = file_open(&cf_path, name, "r");

  cfdev->name = strdup(file_get_fqname());
  cfdev->lsn = cfdev->max_lsn = 0L;
  cfdev->count = cfdev->index = 0;

  if (cfdev->fd) {
    long endpos;

    // reopen the file for writing
    file_close(cfdev->fd);
    cfdev->fd = fopen(cfdev->name, "a");

    fseek(cfdev->fd, 0, SEEK_END);
    endpos = ftell(cfdev->fd);
    cfdev->max_lsn = endpos / BYTES_PER_SECTOR;
    //printf("MAX LSN = %ld\n", cfdev->max_lsn);
  }
}

void calculate_chs_values(struct cf_controller *cfdev)
{
  long lsn = 0;
  int heads = 16;
  int spt = 256;
  int cyl;
  int target_rem = 0;

  if (cfdev->fd) {
    lsn = cfdev->max_lsn;
  }

  debugf(20, "Biggest LSN is %ld\n", lsn);
  while (target_rem < 1024) {
    for (heads=16; heads; heads--) {
      if (heads<<24 >= lsn) {
	int quot = lsn / heads;
	int rem = lsn % heads;

	debugf(20, "Trying with %d heads (target_rem=%d)\n", heads, target_rem);
	
	debugf(20, "%d / %d is %d rem %d\n", lsn, heads, quot, rem);

	if (rem <= target_rem) {
	  debugf(20, "heads=%d is a candidate: rem=%d\n", heads, rem);

	  for (spt=256; spt; spt--) {
	    int q = quot / spt;
	    int r = quot % spt;

	    debugf(50, "... %d:d\n", q, spt); 

	    if (r == 0) {
	      debugf(20, "Perfect CHS=%d:%d:%d found\n", q, heads, spt);
	      cfdev->num_heads = heads;
	      cfdev->sectors_per_track = spt;
	      cfdev->num_cylinders = q;

	      return;
	    }
	  }
	}
      }
      else {
	debugf(20, "%d heads is not enough\n", heads);
      }
    }

    target_rem = (target_rem)?target_rem<<1:1;
  }

  heads = 16;
  cyl = lsn / (heads * spt);

  debugf(20, "Calculated CHS=%d:%d:%d\n", cyl, heads, spt);
  cfdev->num_heads = heads;
  cfdev->sectors_per_track = spt;
  cfdev->num_cylinders = cyl;
}

struct hw_device* compact_flash_create (void)
{
  struct cf_controller *cfdev = malloc (sizeof (struct cf_controller));

  if (controllerId == 0) {
    char *cf = getenv("CFDIR");
    
    path_init(&cf_path);

    if (cf != NULL) {
      path_add(&cf_path, cf);
    }

    path_add(&cf_path, "Images");
  }

  connect_to_disk_file(cfdev, 0);
  //connect_to_disk_file(cfdev, 1);

  calculate_chs_values(cfdev);

  cfdev->controllerId = controllerId;
  controllerId++;

  cfdev->disk_id = 0;
  cfdev->eight_bit_mode = 0;
  cfdev->status_reg = SR_DSC | SR_DRQ;

  return device_attach (&compact_flash_class, 4, cfdev);
}
