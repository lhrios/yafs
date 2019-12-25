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
 * FAT Device Module: deals with FAT Devices writing and reading the file system.
 */

#ifndef __FAT_DEVICE_H
#define __FAT_DEVICE_H

	#include "exception.h"
	#include "fat.h"
	#include "fat_device_type.h"
	#include "fat_elements.h"
	#include "file_io.h"
	#include "types.h"

	#include <vector>
	#include <string>
	using namespace std;

	class FATDevice {
		public:
			FATDevice(const char* path, const char *access_mode);
			~FATDevice();
			RootDirectory* ReadDirectoriesTree();
			void WriteDirectoriesTree(RootDirectory* );

			operator string();

         class FATDeviceException : public Exception {
				public:
					FATDeviceException(string message = ""):Exception(message){}
			};

			enum FATType {
				FAT12 = 0,
				FAT16 = 1,
				FAT32 = 2
			};

		private:
			FileIO *device_file;
			BootSectorBIOSParameterBlock bs_bpb;
			BIOSParameterBlockFAT32 *bpb_fat32;
			BootSectorFAT bs_fat;
			uint32 fat_size , first_data_sector , sectors_root_directory , total_sectors ,
				data_sectors , total_clusters , cluster_size , fat_buffer_sector;
			vector<uint32> fats_first_sector;
			FATType fat_type;
			uint8 *fat_buffer;

			static uint32 file_last_cluster[];

			bool IsLastCluster(uint32 cluster){
				cluster = cluster & 0x0FFFFFFF;
				return cluster >= file_last_cluster[(uint32)fat_type] || cluster == 0;
			}
			void ReadDirectory(FATDirectory*);
			void WriteDirectory(FATDirectory*);
			uint64 GetClusterOffset(uint32 cluster);
			void ReadSector(void* buffer , uint32 sector);
			void ReadCluster(void* buffer , uint32 cluster);
			void WriteSector(void* buffer , uint32 sector);
			void WriteCluster(void* buffer , uint32 cluster);
			uint32 ReadFAT(uint32 cluster);

			friend ostream &operator<<(ostream &stream , FATDevice &fat_device);
			void ThrowNotFATFileSystemException(){
				throw FATDeviceException("The device does not have a FAT file system.");
			}
	};

	ostream &operator<<(ostream &stream , FATDevice &fat_device);
#endif
