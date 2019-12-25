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
 * Unicode Module: defines some functions to deal with unicode encodings.
 */

#ifndef YAFS_UNICODE_H
	#define YAFS_UNICODE_H

	#include "exception.h"
	#include "types.h"

	#include <vector>
	using namespace std;

	class Unicode {
		public:
         class UnicodeException : public Exception {
				public:
					UnicodeException(string message = ""):Exception(message){}
			};
			static char* ConvertFromByteToUTF8(const char* text);
			static char* ConvertFromUTF16ToUTF8(const wchar_t* text);		
			static vector<uint8> ConvertFromUTF16ToUTF8(vector<uint16> text_utf16);
			static vector<uint8> ConvertFromByteToUTF8(vector<uint8> text_byte);
	};

#endif
