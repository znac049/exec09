/*
 * Copyright 2008 by Brian Dominy <brian@oddchange.com>
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

#ifndef _MACHINE_GTRON_H
#define _MACHINE_GTRON_H

/* This file defines the characteristics of the GOONATRON machine architecture.
EON is a little more advanced than the 'simple' architecture that runs by
default, and can be used to run more sophiscated programs for the 6809.
However, no actual hardware resembling this exists.

Each I/O device comprises 128 bytes of address space.  There can be up
to a maximum of 8 devices defined this way.  The "device ID" is just the
offset from the beginning of the I/O region.  At present, 5 devices are
defined:

*/

#define RAM_SIZE (56*1024)
#define RAM_BASE 0

#define ROM_SIZE 8192
#define ROM_BASE 0xe000


#endif /* _MACHINE_EON_H */
