#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "machine.h"

struct LEDs_port {
  int x;
};

U8 LEDs_read (struct hw_device *dev, unsigned long addr)
{
  struct LEDs_port *port = (struct LEDs_port *)dev->priv;

  return 42;
}

void LEDs_write (struct hw_device *dev, unsigned long addr, U8 val)
{
  struct LEDs_port *port = (struct LEDs_port *)dev->priv;
}

void LEDs_reset (struct hw_device *dev)
{
  struct LEDs_port *port = (struct LEDs_port *)dev->priv;

  port->x = 0;
}

struct hw_class LEDs_class =
  {
    .name = "LEDs",
    .readonly = 0,
    .reset = LEDs_reset,
    .read = LEDs_read,
    .write = LEDs_write,
  };

extern U8 null_read (struct hw_device *dev, unsigned long addr);

struct hw_device* leds_create (void)
{
  struct LEDs_port *port = malloc (sizeof (struct LEDs_port));
  port->x = 0;
  return device_attach (&LEDs_class, 16, port);
}
