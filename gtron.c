#include <stdio.h>
#include <fcntl.h>
#include "machine.h"

#include "gtron.h"


/**
 * Initialize the 'goonatron' machine
 */
void gtron_init (const char *boot_rom_file)
{
  struct hw_device *iodev;
  struct hw_device *uart;
  struct hw_device *cfdev;

  printf("GTron init: rom='%s'\n", boot_rom_file);

  device_define(ram_create(RAM_SIZE), 0,
		  RAM_BASE, RAM_SIZE, MAP_READWRITE);

  device_define(rom_create(boot_rom_file, ROM_SIZE), 0,
		  ROM_BASE, ROM_SIZE, MAP_READABLE);

  iodev = ioexpand_create();
  uart = mc6850_create();
  cfdev = compact_flash_create();

  ioexpand_attach(iodev, 0, 0x00, uart);
  ioexpand_attach(iodev, 2, 0x00, cfdev);
  device_define(iodev, 0, 0xff00, 128, MAP_READWRITE);
}


struct machine gtron_machine =
  {
    .name = "gtron",
    .fault = fault,
    .init = gtron_init,
    .periodic = 0,
  };
