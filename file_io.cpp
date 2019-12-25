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

#include "file_io.h"
#include "types.h"
#include "utils.h"

#include <cassert>
#include <cstring>

#include <cstdio>

#include <sstream>

using namespace std;

/* Windows. */
#ifdef WIN_SYSTEM
   const uint32 FileIO::IO_SEEK_SET = FILE_BEGIN;
   const uint32 FileIO::IO_SEEK_CUR = FILE_CURRENT;
   const uint32 FileIO::IO_SEEK_END = FILE_END;

/* Unix. */
#elif UNIX_SYSTEM
   #ifndef _LARGEFILE64_SOURCE
		#define _LARGEFILE64_SOURCE
	#endif
   #include <unistd.h>
	#include <sys/types.h>
	#include <sys/stat.h>
	#include <fcntl.h>
	#include <errno.h>

   const uint32 FileIO::IO_SEEK_SET = SEEK_SET;
   const uint32 FileIO::IO_SEEK_CUR = SEEK_CUR;
   const uint32 FileIO::IO_SEEK_END = SEEK_END;
#endif

const uint32 FileIO::READ_MODE = 0x1;
const uint32 FileIO::WRITE_MODE = 0x2;

FileIO::FileIO(const char* path , const char* mode , bool lock){

	if(mode == NULL) throw FileIOException("The second parameter is invalid.");

	/* Windows. */
	#ifdef WIN_SYSTEM
		DWORD  creation_disposition = 0 , flags , unused, dwFlagsAndAttributes = FILE_ATTRIBUTE_SYSTEM;

		if(!strcmp(mode , "r")){
			flags = FILE_GENERIC_READ;
			creation_disposition = OPEN_EXISTING;
			this->mode = READ_MODE;
		}else if(!strcmp(mode , "w")){
			flags = FILE_GENERIC_WRITE;
			creation_disposition = CREATE_ALWAYS;
			this->mode = WRITE_MODE;
		}else if(!strcmp(mode , "r+")){
			flags = FILE_READ_DATA | FILE_WRITE_DATA;
			creation_disposition = OPEN_EXISTING;
			this->mode = READ_MODE | WRITE_MODE;
			dwFlagsAndAttributes |= FILE_FLAG_WRITE_THROUGH;
		}else if(!strcmp(mode , "w+")){
			flags = FILE_READ_DATA | FILE_WRITE_DATA;
			creation_disposition = CREATE_ALWAYS;
			this->mode = READ_MODE | WRITE_MODE;
			dwFlagsAndAttributes = FILE_FLAG_WRITE_THROUGH;
		}else throw FileIOException("The second parameter is invalid.");

		dwFlagsAndAttributes |= FILE_FLAG_NO_BUFFERING;

		if(INVALID_HANDLE_VALUE == (file = CreateFile((LPCTSTR)path , flags , FILE_SHARE_WRITE
				, NULL , creation_disposition , dwFlagsAndAttributes , NULL))){
			throwIOExceptionWithErrorCode(string("Error while opening the file \"") + path + "\".");
		}
		if(lock && 0 == DeviceIoControl(file , (DWORD)FSCTL_LOCK_VOLUME , NULL , 0 , NULL , 0 , (LPDWORD)&unused , NULL)){
			throwIOExceptionWithErrorCode(string("Error while locking the file \"") + path + "\".");
		}
	/* Unix. */
	#elif UNIX_SYSTEM
		int flags;

		if(!strcmp(mode , "r")){
			flags = O_RDONLY;
			this->mode = READ_MODE;
		}else if(!strcmp(mode , "w")){
			flags = O_WRONLY | O_CREAT | O_TRUNC;
			this->mode = WRITE_MODE;
		}else if(!strcmp(mode , "r+")){
			flags = O_RDWR;
			this->mode = READ_MODE | WRITE_MODE;
		}else if(!strcmp(mode , "w+")){
			flags = O_RDWR | O_TRUNC | O_CREAT;
			this->mode = READ_MODE | WRITE_MODE;
		}else throw FileIOException("The second parameter is invalid.");

      file = open(path , flags , S_IRUSR | S_IWUSR | O_LARGEFILE);
      if(file == -1) throwIOExceptionWithErrorCode(string("Error while opening the file \"") + path + "\".");

   #endif
}

void FileIO::Close(){
	/* Windows. */
	#ifdef WIN_SYSTEM
		DWORD unused;
		if (mode & WRITE_MODE) {
			if (FlushFileBuffers(file) == 0) {
				throwIOExceptionWithErrorCode("Error while closing the file.");
			}

			if(DeviceIoControl(file, (DWORD) FSCTL_DISMOUNT_VOLUME, NULL, 0, NULL, 0, &unused, NULL) == 0) {
				throwIOExceptionWithErrorCode("Error while dismouting the file.");
			}
		}

		if(CloseHandle((HANDLE)file) == 0)
			throwIOExceptionWithErrorCode("Error while closing the file.");
	/* Unix. */
	#elif UNIX_SYSTEM
		if(fsync(file) != 0)
			throwIOExceptionWithErrorCode("Error while closing the file.");

		if(close(file) != 0)
			throwIOExceptionWithErrorCode("Error while closing the file.");
   #endif
}

uint32 FileIO::ReadInternal(void* buffer , uint32 count){
	uint32 bytes_read = 0;

	if (mode & READ_MODE) {
	/* Windows. */
	#ifdef WIN_SYSTEM
		BOOL success;
		DWORD aux;
		success = ReadFile((HANDLE)file , (LPVOID)buffer , count , &aux , NULL);
		if (!success || aux != count)
         throwIOExceptionWithErrorCode("Error while reading the file.");
		bytes_read = aux;
	/* Unix. */
	#elif UNIX_SYSTEM
		ssize_t aux = read(file , buffer , count);
		if(aux == (ssize_t)-1 || aux != count)
			throwIOExceptionWithErrorCode("Error while reading the file.");
		bytes_read = aux;
   #endif
	} else {
		throw FileIOException("File without read mode activated.");
	}

	return bytes_read;
}

uint32 FileIO::Read(void *buffer , uint32 count , uint64 offset){
	if (LogUtils::IsEnabled()) {
		LogUtils::Debug() << "Reading " << count << " bytes from 0x" << hex << offset << dec << "." << endl;
	}

	SeekInternal(offset , IO_SEEK_SET);
	return ReadInternal(buffer , count);
}

uint32 FileIO::WriteInternal(const void* buffer , uint32 count){
   uint32 bytes_written = 0;

	if (mode & WRITE_MODE) {
		/* Windows. */
		#ifdef WIN_SYSTEM
			BOOL success;
			DWORD aux;
			success = WriteFile((HANDLE)file , (LPVOID)buffer , count , &aux , NULL);
			if (!success || aux != count)
				throwIOExceptionWithErrorCode("Error while writing in the file.");
			bytes_written = aux;
		/* Unix. */
		#elif UNIX_SYSTEM
			ssize_t aux;
			aux = write(file , buffer , count);
			if(aux == (ssize_t)-1 || aux != count)
				throwIOExceptionWithErrorCode("Error while writing in the file.");
			bytes_written = aux;
		#endif
	} else {
		throw FileIOException("File without write mode activated.");
	}

   return bytes_written;
}

uint32 FileIO::Write(const void *buffer , uint32 count , uint64 offset){
	if (LogUtils::IsEnabled()) {
		LogUtils::Debug() << "Writing " << count << " bytes on 0x" << hex << offset << dec << "." << endl;
	}

	SeekInternal(offset , IO_SEEK_SET);
	return WriteInternal(buffer , count);
}

void FileIO::SeekInternal(int64 offset , uint32 mode){
	/* Windows. */
	#ifdef WIN_SYSTEM
      LARGE_INTEGER li_offset , aux;

      li_offset.QuadPart = offset;
      if(!SetFilePointerEx((HANDLE)file , li_offset , &aux , mode ))
			throwIOExceptionWithErrorCode("Error while seeking the file.");
	/* Unix. */
	#elif UNIX_SYSTEM
		off64_t aux;
		if((aux = lseek64(file , offset , mode)) == (off64_t)-1)
			throwIOExceptionWithErrorCode("Error while seeking the file.");
   #endif
}

void FileIO::throwIOExceptionWithErrorCode(string message) {
	stringstream sstream;

	/* Windows. */
	#ifdef WIN_SYSTEM
		sstream << message << " Error code: " << GetLastError() << ".";

	/* Unix. */
	#elif UNIX_SYSTEM
		sstream << message << " Error code: " << errno << ".";
   #endif

	throw FileIOException(sstream.str());
}
