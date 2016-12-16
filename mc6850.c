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
#include <assert.h>
#include "machine.h"

/* Emulate a serial port.  Basically this driver can be used for any byte-at-a-time
   input/output interface. */
struct mc6850_port
{
  unsigned int ctrl;
  unsigned int status;
  int fin;
  int fout;
};

/* The I/O registers exposed by this driver */
#define SER_CTL_STATUS   0     /* Control (write) and status (read) */
#define SER_DATA         1     /* Data input/output */

#define SER_CTL_RESET   0x03   /* CR1:0=11 - Reset device */

#define SER_STAT_READOK  0x1
#define SER_STAT_WRITEOK 0x2
#define SER_STAT_TXEMPTY 0x2

void mc6850_update (struct mc6850_port *port)
{
  fd_set infds, outfds;
  struct timeval timeout;
  int rc;

  FD_ZERO (&infds);
  FD_SET (port->fin, &infds);
  FD_ZERO (&outfds);
  FD_SET (port->fout, &outfds);
  timeout.tv_sec = 0;
  timeout.tv_usec = 0;
  rc = select (2, &infds, &outfds, NULL, &timeout);
  if (FD_ISSET (port->fin, &infds))
    port->status |= SER_STAT_READOK;
  else
    port->status &= ~SER_STAT_READOK;
  if (FD_ISSET (port->fout, &outfds))
    port->status |= SER_STAT_WRITEOK;
  else
    port->status &= ~SER_STAT_WRITEOK;
}

U8 mc6850_read (struct hw_device *dev, unsigned long addr)
{
  struct mc6850_port *port = (struct mc6850_port *)dev->priv;
  int retval;
  mc6850_update (port);
  switch (addr)
    {
    case SER_DATA:
      {
	U8 val;
	if (!(port->status & SER_STAT_READOK))
	  return 0xFF;
	retval = read (port->fin, &val, 1);
	assert(retval != -1);
	return val;
      }
    case SER_CTL_STATUS:
      return port->status;
    }
}

void mc6850_write (struct hw_device *dev, unsigned long addr, U8 val)
{
  struct mc6850_port *port = (struct mc6850_port *)dev->priv;
  int retval;

  switch (addr) {
  case SER_DATA:
    {
      U8 v = val;
      retval = write (port->fout, &v, 1);
      assert(retval != -1);
      break;
    }
  case SER_CTL_STATUS:
    port->ctrl = val;
    break;
  }
}

void mc6850_reset (struct hw_device *dev)
{
  struct mc6850_port *port = (struct mc6850_port *)dev->priv;
  port->ctrl = 0;
  port->status = SER_STAT_TXEMPTY;;
}

struct hw_class mc6850_class =
  {
    .name = "mc6850",
    .readonly = 0,
    .reset = mc6850_reset,
    .read = mc6850_read,
    .write = mc6850_write,
  };

extern U8 null_read (struct hw_device *dev, unsigned long addr);

struct hw_device* mc6850_create (void)
{
  struct mc6850_port *port = malloc (sizeof (struct mc6850_port));
  port->fin = STDIN_FILENO;
  port->fout = STDOUT_FILENO;
  return device_attach (&mc6850_class, 4, port);
}
