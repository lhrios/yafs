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

#include "types.h"
#include "utils.h"

#include <vector>

#ifndef _MSC_VER
	#include <unistd.h>
#else
	#define WIN32_LEAN_AND_MEAN
	#include "windows.h"
	#undef WIN32_LEAN_AND_MEAN
#endif

string WorkingDirectoryUtils::working_directory;
bool WorkingDirectoryUtils::working_directory_was_loaded = false;

string WorkingDirectoryUtils::GetWorkingDirectory(){
	if(!working_directory_was_loaded){
		uint8 buffer[4096];

		/* GCC. */
		#ifdef __GNUC__
			getcwd((char*) buffer , 4096);
		/* Microsoft Compiler. */
		#elif _MSC_VER
			GetCurrentDirectory(4096, LPTSTR(buffer));
		#endif

		working_directory_was_loaded = true;
		#ifdef WIN_SYSTEM
			working_directory = string((char*)buffer) + "\\";
		#elif UNIX_SYSTEM
			working_directory = string((char*)buffer) + "/";
		#endif

	}
	return working_directory;
}

bool LogUtils::enabled = false;
