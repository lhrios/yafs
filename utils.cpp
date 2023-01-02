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
#include "unicode.h"

#include <algorithm>
#include <cstring>
#include <iostream>

#ifndef _MSC_VER
	#include <unistd.h>
#else
	#define WIN32_LEAN_AND_MEAN
	#include "windows.h"
	#undef WIN32_LEAN_AND_MEAN
#endif

string ExecutableDirectoryUtils::executable_directory_uri_utf8;
string ExecutableDirectoryUtils::executable_directory_utf8;
string ExecutableDirectoryUtils::executable_directory_native_encoding;			

bool LogUtils::enabled = false;

#ifdef UNIX_SYSTEM
	void handleFatalError(int status, int errnum, const char* message) {
		cerr << "yafs: " << message << endl;
		exit(status);
	}
#endif

void ExecutableDirectoryUtils::Initialize(char* command_line_executable_path){
	#ifdef WIN_SYSTEM
		char executable_path_ansi[4096];
		GetModuleFileNameA(NULL, executable_path_ansi, sizeof(executable_path_ansi));	
		
		wchar_t executable_path_unicode[4096];
		GetModuleFileNameW(NULL, executable_path_unicode, sizeof(executable_path_unicode));				
		
		/* Removes the executable name. */
		int len = (int) strlen(executable_path_ansi);
		int i;
		for (i = len - 1; i >= 0; i--) {
			if (executable_path_ansi[i] == '\\') break;
		}
		if (i > -1) {
			executable_path_ansi[i + 1] = '\0';
		}

		len = (int) wcslen(executable_path_unicode);
		for (i = len - 1; i >= 0; i--) {
			if (executable_path_unicode[i] == '\\') break;
		}
		if (i >= -1) {
			executable_path_unicode[i + 1] = '\0';
		}

		executable_directory_native_encoding = string(executable_path_ansi);
		
		char* executable_path_utf8 = Unicode::ConvertFromUTF16ToUTF8(executable_path_unicode);
		executable_directory_utf8 = string((char*)executable_path_utf8);
		delete[] executable_path_utf8;
		
		executable_directory_uri_utf8 = string("file://") + string("/") + executable_directory_utf8;
		std::replace(executable_directory_uri_utf8.begin(), executable_directory_uri_utf8.end(), '\\', '/');
	#elif UNIX_SYSTEM
		char working_directory[4096];
		working_directory[0] = '\0';

		char* result = getcwd(working_directory, sizeof(working_directory));
		if (result != NULL) {
			handleFatalError(EXIT_FAILURE, errno, "failed after executing getcwd");
		}
		size_t working_directory_length = strlen(working_directory);	
		if (working_directory[working_directory_length - 1] != '/') {
			working_directory[working_directory_length++] = '/';
		}
		
		if (command_line_executable_path != NULL) {	  
			size_t command_line_executable_path_length = strlen(command_line_executable_path);

			/* Removes the executable name. */
			int i;
			for (i = command_line_executable_path_length - 1; i >= 0; i--) {
				if (command_line_executable_path[i] == '/') break;
			}
			if (i > -1) {
				command_line_executable_path[i + 1] = '\0';
				command_line_executable_path_length = i + 1;
			}			
			
			/* Is it not absolute? */
			if (command_line_executable_path[0] != '/') {
				memcpy(working_directory + working_directory_length, command_line_executable_path, command_line_executable_path_length + 1);
				working_directory_length += command_line_executable_path_length;

			} else {
				memcpy(working_directory, command_line_executable_path, command_line_executable_path_length + 1);
				working_directory_length = command_line_executable_path_length;
			}
		}

		/* Assumes it is using UFT-8. */		
		executable_directory_utf8 = string(working_directory);

		if (executable_directory_utf8[executable_directory_utf8.length() - 1] != '/'){
			executable_directory_utf8 += "/";
		}

		executable_directory_uri_utf8 = string("file://") + executable_directory_utf8;
		executable_directory_native_encoding = executable_directory_utf8;
	#endif
}
