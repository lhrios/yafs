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
 * Util Module: groups some unrelated important functions.
 */

#ifndef YAFS_UTIL_MODULE_H
	#define YAFS_UTIL_MODULE_H

	#include "types.h"

	#include <iostream>
	#include <string>
	using namespace std;

	class ExecutableDirectoryUtils {
		public:
			static void Initialize(char* command_line_executable_path);
			static inline string GetExecutableDirectoryURIUFT8(){
				return executable_directory_uri_utf8;
			}

			static inline string GetExecutableDirectoryUFT8(){
				return executable_directory_utf8;
			}

			static inline string GetExecutableDirectoryNativeEncoding(){
				return executable_directory_native_encoding;
			}
		private:
			static string executable_directory_uri_utf8;
			static string executable_directory_utf8;
			static string executable_directory_native_encoding;			
	};

	class LogUtils {
		public:
			static bool IsEnabled() {
				return enabled;
			}
			static void SetEnabled(bool enabled) {
				LogUtils::enabled = enabled;
			}
			static ostream& Debug() {
				return cout << "DEBUG: ";
			}

		private:
			static bool enabled;
	};

#endif
