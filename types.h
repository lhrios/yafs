/*
 * Copyright 2009 Luis Henrique O. Rios
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

/**
 * Types Module: defines some types that will be useful.
 */

#ifndef YAFS_TYPES_H
	#define YAFS_TYPES_H

	typedef unsigned char uint8;
	#ifndef __APPLE__
		static_assert(sizeof(uint8) == 1, "Expecting uint8 with 1 byte length");
	#endif

	typedef unsigned short int uint16;
	#ifndef __APPLE__
		static_assert(sizeof(uint16) == 2, "Expecting uint16 with 2 byte length");
	#endif

	typedef unsigned int uint32 , uint;
	#ifndef __APPLE__
		static_assert(sizeof(uint32) == 4, "Expecting uint32 with 4 byte length");
	#endif

	typedef unsigned long long int uint64;
	#ifndef __APPLE__
		static_assert(sizeof(uint64) == 8, "Expecting uint64 with 8 byte length");
	#endif

	typedef char int8;
	typedef short int int16;
	typedef long long int int64;

#endif
