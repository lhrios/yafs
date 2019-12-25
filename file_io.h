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
 * I/O Module: defines some functions to do I/O using files.
 */

#ifndef __IO_H
#define __IO_H

	#include "exception.h"
	#include "types.h"

	#include <string>

	using namespace std;

	/* Windows. */
	#ifdef WIN_SYSTEM
		#ifndef _WIN32_WINNT
			#define _WIN32_WINNT 0x0500
		#endif

		#define WIN32_LEAN_AND_MEAN
		#include "windows.h"
		#include "winioctl.h"
		#undef WIN32_LEAN_AND_MEAN
		#undef _WIN32_WINNT

		typedef HANDLE File;

	/* Unix. */
	#elif UNIX_SYSTEM
		typedef int File;
	#endif

   class FileIO {
      public:
         FileIO(const char *path , const char *mode , bool lock = true);

			uint32 Read(void *buffer , uint32 count , uint64 offset);
			uint32 Write(const void *buffer , uint32 count , uint64 offset);

         ~FileIO(){
            Close();
         }

         class FileIOException : public Exception {
				public:
					FileIOException(string message = ""):Exception(message){}
			};

         const static uint32 IO_SEEK_SET , IO_SEEK_CUR , IO_SEEK_END;
			const static uint32 READ_MODE , WRITE_MODE;

      private:
         FileIO();
         FileIO(const FileIO&);
         FileIO& operator=(const FileIO&);
         void Close();

			uint32 ReadInternal(void* buffer , uint32 count);
			uint32 WriteInternal(const void* buffer , uint32 count);
			void SeekInternal(uint64 offset , uint32 mode);

			void throwIOExceptionWithErrorCode(string message);

         File file;
         uint32 mode;
   };

#endif
