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
  struct hw_device *iodev_two;
  struct hw_device *uart;
  struct hw_device *cfdev;
  struct hw_device *sevensegdev;
  struct hw_device *sddev;
  struct hw_device *romdev;

  printf("GTron init: rom='%s'\n", boot_rom_file);

  device_define(ram_create(RAM_SIZE), 0,
		  RAM_BASE, RAM_SIZE, MAP_READWRITE);

  romdev = rom_create(boot_rom_file, ROM_SIZE);
  device_define(romdev, 0, ROM_BASE, ROM_SIZE, MAP_READABLE);

  iodev = ioexpand_create();
  iodev_two = ioexpand_create();

  uart = mc6850_create();
  cfdev = compact_flash_create();
  sevensegdev = leds_create();

  sddev = sdcard_create();

  ioexpand_attach(iodev, 0, 0x00, uart);
  ioexpand_attach(iodev, 2, 0x00, cfdev);
  ioexpand_attach(iodev, 14, 0x00, sevensegdev);

  ioexpand_attach(iodev_two, 11, 0x00, sddev);
  ioexpand_attach(iodev_two, 14, ROM_SIZE-16, romdev);
  ioexpand_attach(iodev_two, 15, ROM_SIZE-8, romdev);

  device_define(iodev, 0, 0xff00, 128, MAP_READWRITE);
  device_define(iodev_two, 0, 0xff80, 128, MAP_READWRITE);
}


struct machine gtron_machine =
  {
    .name = "gtron",
    .fault = fault,
    .init = gtron_init,
    .periodic = 0,
  };
