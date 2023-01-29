/*
 * Copyright 2013 Luis Henrique O. Rios
 *
 * This file is part of YAFS.
 *
 * YAFS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * YAFS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with YAFS.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef YAFS_PACK_H
	#define YAFS_PACK_H

	/* Windows. */
	#ifdef WIN_SYSTEM
		/* GCC. */
		#ifdef __GNUC__
			#define PACK(D) D __attribute__((__packed__))

		/* Microsoft Compiler. */
		#elif _MSC_VER
			#define PACK(D) __pragma(pack(push, 1)) D __pragma(pack(pop))

		#else
			#error "Unknown compiler"
		#endif

	/* Unix. */
	#elif UNIX_SYSTEM
		#define PACK(D) D __attribute__((__packed__))
	#endif

#endif
