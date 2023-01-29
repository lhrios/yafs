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

#include "fat_device.h"
#include "file_io.h"
#include "types.h"

#include <cassert>
#include <cstring>
#include <memory>
#include <sstream>
#include <string>
using namespace std;

uint32 FATDevice::file_last_cluster[] = {
	0x00000FF8,
	0x0000FFF8,
	0x0FFFFFF8
};

uint8 ComputeCheckSum(uint8 *name){
   uint32 i;
   uint8 sum = 0;

   for(i = 11 ; i != 0 ; i--){
      sum = ((sum & 1) ? 0x80 : 0) + (sum >> 1) + *name++;
   }
   return sum;
}

FATDevice::FATDevice(const char *path, const char *access_mode){
	std::unique_ptr<uint8[]> sector_buffer = std::unique_ptr<uint8[]>(new uint8[4096]);
	try{
		device_file = new FileIO(path , access_mode, true);

		device_file->Read(sector_buffer.get() , 4096, 0);
		memcpy(&bs_bpb , sector_buffer.get() , sizeof(BootSectorBIOSParameterBlock));

		/* It will do some tests to check if the device has a FAT file system. */
		/* Check BS_jmpBoot. */
		if(!((bs_bpb.BS_jmpBoot[0] == 0xEB && bs_bpb.BS_jmpBoot[2] ==  0x90) ||
         (bs_bpb.BS_jmpBoot[0] == 0xE9)))
			ThrowNotFATFileSystemException();

		/* Check BPB_BytsPerSec. */
		if(bs_bpb.BPB_BytsPerSec != 512 && bs_bpb.BPB_BytsPerSec != 1024 &&
			bs_bpb.BPB_BytsPerSec != 2048 && bs_bpb.BPB_BytsPerSec != 4096)
			ThrowNotFATFileSystemException();

		/* Check BPB_SecPerClus. */
		if(bs_bpb.BPB_SecPerClus != 1 && bs_bpb.BPB_SecPerClus != 2 &&
			bs_bpb.BPB_SecPerClus != 4 && bs_bpb.BPB_SecPerClus != 8 &&
			bs_bpb.BPB_SecPerClus != 16 && bs_bpb.BPB_SecPerClus != 32 &&
			bs_bpb.BPB_SecPerClus != 64 && bs_bpb.BPB_SecPerClus != 128)
			ThrowNotFATFileSystemException();

		/* Check BPB_RsvdSecCnt. */
		if(bs_bpb.BPB_RsvdSecCnt == 0)
			ThrowNotFATFileSystemException();

		/* Check the number of FATs. */
		if(bs_bpb.BPB_NumFATs == 0) ThrowNotFATFileSystemException();

		/* Check BPB_Media. The value here must be equal to the least
			significant byte at FAT[0]. This will be checked later.*/
		if(bs_bpb.BPB_Media != 0xF0 && bs_bpb.BPB_Media != 0xF8 &&
			bs_bpb.BPB_Media != 0xF9 && bs_bpb.BPB_Media != 0xFA &&
			bs_bpb.BPB_Media != 0xFB && bs_bpb.BPB_Media != 0xFC &&
			bs_bpb.BPB_Media != 0xFD && bs_bpb.BPB_Media != 0xFE &&
			bs_bpb.BPB_Media != 0xFF)
			ThrowNotFATFileSystemException();

		/* If BPB_FATSz16 == 0 then BPB_FATSz32 is used. */
		if(bs_bpb.BPB_FATSz16 == 0){
         bpb_fat32 = new BIOSParameterBlockFAT32();
         memcpy(bpb_fat32 , sector_buffer.get() + sizeof(BootSectorBIOSParameterBlock) ,
				sizeof(BIOSParameterBlockFAT32));
         fat_size = bpb_fat32->BPB_FATSz32;
		}else{
         bpb_fat32 = NULL;
			fat_size = bs_bpb.BPB_FATSz16;
		}

		/* Calculate the number of sector in root directory. */
		// TODO: Division remainder must be zero.
		sectors_root_directory = (bs_bpb.BPB_RootEntCnt * DIR_ENTRY_SIZE) / bs_bpb.BPB_BytsPerSec;

		/* Calculate the first data sector. */
		first_data_sector = bs_bpb.BPB_RsvdSecCnt + (bs_bpb.BPB_NumFATs * fat_size) +
			sectors_root_directory;

		/* Calculate the FATs first sector. */
		{
			uint32 aux = bs_bpb.BPB_RsvdSecCnt , i;
			for(i = 0 ; i < bs_bpb.BPB_NumFATs ; i++){
				fats_first_sector.push_back(aux);
				aux += fat_size;
			}
		}

		/* Calculate the total number of sectors. */
		if(bs_bpb.BPB_TotSec16 != 0){
			total_sectors = bs_bpb.BPB_TotSec16;
		}else if(bs_bpb.BPB_TotSec32 != 0){
			total_sectors = bs_bpb.BPB_TotSec32;
		}else ThrowNotFATFileSystemException();

		/* Calculate the number of bytes per cluster. */
		cluster_size = bs_bpb.BPB_BytsPerSec * bs_bpb.BPB_SecPerClus;

		/* Calculate the number of sectors used to store the data. */
		data_sectors = total_sectors -
			(bs_bpb.BPB_RsvdSecCnt + (bs_bpb.BPB_NumFATs * fat_size) + sectors_root_directory);

		/* Calculate the number of clusters (set of sectors). */
		total_clusters = data_sectors / bs_bpb.BPB_SecPerClus;

		/* Determine the FAT type and also do some security verifications. */
		/* FAT 12. */
		if(total_clusters < 4085){
			if(((bs_bpb.BPB_RootEntCnt * 32) / bs_bpb.BPB_BytsPerSec) % 2 != 0 ||
				(bs_bpb.BPB_TotSec16 == 0 && bs_bpb.BPB_TotSec32 == 0))
				ThrowNotFATFileSystemException();
			fat_type = FAT12;
			throw FATDeviceException("The program can not deal with FAT12 file system.");

		/* FAT 16. */
		}else if(total_clusters < 65525){
			if((((bs_bpb.BPB_RootEntCnt * DIR_ENTRY_SIZE) / bs_bpb.BPB_BytsPerSec) % 2 != 0) ||
				(bs_bpb.BPB_TotSec16 == 0 && bs_bpb.BPB_TotSec32 == 0))
				ThrowNotFATFileSystemException();
			fat_type = FAT16;

		/* FAT 32. */
		}else{
			if(bs_bpb.BPB_RootEntCnt != 0 ||	bs_bpb.BPB_TotSec16 != 0 ||
				bs_bpb.BPB_FATSz16 != 0)
				ThrowNotFATFileSystemException();
			fat_type = FAT32;
		}

		/* Read the BIOS Parameter Block. */
		if(fat_type == FAT32){
         memcpy(&bs_fat ,
				sector_buffer.get() + sizeof(BootSectorBIOSParameterBlock) + sizeof(BIOSParameterBlockFAT32) ,
            sizeof(BootSectorFAT));
			if(bpb_fat32 == NULL || strncmp("FAT32   " , (char*)bs_fat.BS_FilSysType , 8))
				ThrowNotFATFileSystemException();
			if(bpb_fat32->BPB_FSVer) throw FATDeviceException("Incompatible version of FAT32 file system.");
		}else{
         memcpy(&bs_fat , sector_buffer.get() + sizeof(BootSectorBIOSParameterBlock) ,
            sizeof(BootSectorFAT));
			if((strncmp("FAT12   " , (char*)bs_fat.BS_FilSysType , 8) &&
				strncmp("FAT16   " , (char*)bs_fat.BS_FilSysType , 8) &&
				strncmp("FAT     " , (char*)bs_fat.BS_FilSysType , 8))) ThrowNotFATFileSystemException();
		}

		/* Check the signature. */
		{
			uint16 aux;
         memcpy(&aux , sector_buffer.get() + 510 , sizeof(uint16));
			if(aux != 0xAA55) ThrowNotFATFileSystemException();
		}

		/* Check the FAT[0] entry low byte for each FAT. */
		{
			uint32 i;
			for(i = 0 ; i < bs_bpb.BPB_NumFATs ; i++){
				device_file->Read(sector_buffer.get() , bs_bpb.BPB_BytsPerSec ,
					(int64)fats_first_sector[i] * (int64)bs_bpb.BPB_BytsPerSec);
				if((sector_buffer.get()[0] & 0xFF) != bs_bpb.BPB_Media) ThrowNotFATFileSystemException();
			}
		}

		fat_buffer = new uint8[cluster_size];
		fat_buffer_sector = 0;

	}catch(FileIO::FileIOException f_io_exception){
		throw FATDeviceException(f_io_exception);
	}
}

FATDevice::~FATDevice(){
   if(bpb_fat32 != NULL) delete bpb_fat32;
	delete[] fat_buffer;
   delete device_file;
}

uint64 FATDevice::GetClusterOffset(uint32 cluster){
   assert(total_clusters > cluster && cluster >= 2 );
	/* The clusters number 0 and 1 do not exist so
		the first cluster is the cluster number 2. */
	uint64 offset = (uint64(cluster - 2) * uint64(bs_bpb.BPB_SecPerClus) +
      uint64(first_data_sector)) * uint64(bs_bpb.BPB_BytsPerSec);
   return offset;
}

void FATDevice::ReadSector(void* buffer , uint32 sector){
   uint64 aux;

	assert(sector < total_sectors);
   aux = uint64(sector) * uint64(bs_bpb.BPB_BytsPerSec);
   device_file->Read(buffer , bs_bpb.BPB_BytsPerSec , aux);
}

void FATDevice::ReadCluster(void* buffer , uint32 cluster){
   uint64 aux;

   aux = GetClusterOffset(cluster);
   device_file->Read(buffer , cluster_size , aux);
}

void FATDevice::WriteSector(void* buffer , uint32 sector){
   uint64 aux;

	assert(sector < total_sectors);
   aux = uint64(sector) * uint64(bs_bpb.BPB_BytsPerSec);
   device_file->Write(buffer , bs_bpb.BPB_BytsPerSec , aux);
}

void FATDevice::WriteCluster(void* buffer , uint32 cluster){
   uint64 aux;

   aux = GetClusterOffset(cluster);
   device_file->Write(buffer , cluster_size , aux);
}

uint32 FATDevice::ReadFAT(uint32 cluster){
	uint32 fat_sector , aux = 0;
	uint8 fat_entry_size = fat_type == FAT16 ? 2 : 4;

	fat_sector = fats_first_sector[0] +
		(cluster * fat_entry_size) / bs_bpb.BPB_BytsPerSec;
	/* It its not in the same sector that we have already read. */
	if(fat_sector != fat_buffer_sector || fat_buffer_sector == 0){
		ReadSector(fat_buffer , fat_sector);
		fat_buffer_sector = fat_sector;
	}
	memcpy(&aux , fat_buffer + ((cluster * fat_entry_size) % bs_bpb.BPB_BytsPerSec) ,
		fat_entry_size);
	return aux;
}

void FATDevice::ReadDirectory(FATDirectory* fat_directory){
	bool reading_lde = false;
	FATElement *fat_element;
	GenericEntry *ge;
	vector<LongDirectoryEntryStructure> lde;
	std::unique_ptr<uint8[]> cluster_buffer = std::unique_ptr<uint8[]>(new uint8[cluster_size]);
	uint32 current_cluster = 0;
	uint32 i = 0 , total_entries = 0 , total_lde = 0;
	uint8 current_sum = 0;

	current_cluster = (uint32(fat_directory->directory_entries.back().de.DIR_FstClusHI) << 16) |
		uint32(fat_directory->directory_entries.back().de.DIR_FstClusLO);
	if(IsLastCluster(current_cluster)) return;
	ReadCluster(cluster_buffer.get() , current_cluster);
	ge = (GenericEntry*) cluster_buffer.get();
	
	/* While there are more valid entries. */
	while(ge[i].lde.LDIR_Ord != DIR_ENTRY_END){

      /* If the entry is not empty. */
      if(ge[i].lde.LDIR_Ord != DIR_ENTRY_EMPTY){

			/* If it is a LDE. */
         if((ge[i].lde.LDIR_Attr & ATTR_LONG_NAME_MASK) == ATTR_LONG_NAME){

				/* We are not reading a LDE and we have found the last LDE. */
				if(!reading_lde && (ge[i].lde.LDIR_Ord & LAST_LONG_ENTRY)){
					reading_lde = true;
					total_lde = ge[i].lde.LDIR_Ord & 0xBF;
					lde.push_back(ge[i].lde);
					current_sum = ge[i].lde.LDIR_Chksum;

				/* We are reading a LDE and we have not found the last LDE. */
				}else if(reading_lde && !(ge[i].lde.LDIR_Ord & LAST_LONG_ENTRY) &&
					(ge[i].lde.LDIR_Ord & 0xBFU) == (total_lde - 1) &&
					current_sum == ge[i].lde.LDIR_Chksum){
					total_lde--;
					lde.push_back(ge[i].lde);

				/* We are reading a LDE and we have found the last LDE or
					we are not reading a LDE and we have not found the last LDE.*/
				}else
					throw FATDeviceException("The FAT file system is corrupted.");
			/* If it is a DE. */
			}else{
				if((reading_lde && total_lde == 1 &&
					ComputeCheckSum(ge[i].de.DIR_Name) == current_sum) || !reading_lde){

					reading_lde = false;
					/* Replace the byte 0x05. */
					ge[i].de.DIR_Name[0] = ge[i].de.DIR_Name[0] == 0x05 ? 0xE5 : ge[i].de.DIR_Name[0];
					/* Avoid the special entries "." and ".." .*/
					if(total_entries >= 2){
						fat_element = FATElementFactory::CreateFATElement(&ge[i].de , lde);
						if(fat_element->IsDirectory()) ReadDirectory((FATDirectory*)fat_element);
						fat_directory->InsertFATElement(fat_element);

					} else {
						if (total_entries == 0) {
							fat_directory->dot = ge[i].de;
						} else {
							fat_directory->dotdot = ge[i].de;
						}
					}
					lde.clear();
				}else{
					throw FATDeviceException("The FAT file system is corrupted.");
				}
			}
		}else{
			if(reading_lde)
				throw FATDeviceException("The FAT file system is corrupted.");
		}

      /* Increase the counter. */
      i++;
      total_entries++;
		if(i >= (cluster_size / DIR_ENTRY_SIZE)){
			current_cluster = ReadFAT(current_cluster);
			if(IsLastCluster(current_cluster)) break;
			ReadCluster(cluster_buffer.get() , current_cluster);
			i = 0;
      }
	}
}

RootDirectory* FATDevice::ReadDirectoriesTree(){
	bool reading_lde = false;
	FATElement *fat_element;
	GenericEntry *ge;
	RootDirectory* root_directory = new RootDirectory();
	vector<LongDirectoryEntryStructure> lde;
	std::unique_ptr<uint8[]> cluster_buffer = std::unique_ptr<uint8[]>(new uint8[cluster_size]);
	uint32 current_cluster = 0; /* Used for FAT32. */
	uint32 current_sector = 0; /* Used for FAT12 and FAT16. */
	uint32 i = 0 , total_entries = 0 , total_lde = 0;
	uint8 current_sum = 0;

	/* FAT32. */
   if(fat_type == FAT32){
		current_cluster = bpb_fat32->BPB_RootClus;
		if(IsLastCluster(current_cluster)) return root_directory;
		ReadCluster(cluster_buffer.get() , current_cluster);
	/* FAT12 and FAT16. */
   }else{
		current_sector = fats_first_sector[fats_first_sector.size() - 1] + fat_size;
		ReadSector(cluster_buffer.get() , current_sector);
   }
	ge = (GenericEntry*) cluster_buffer.get();
	
	/* While there are more valid entries. */
	while(ge[i].lde.LDIR_Ord != DIR_ENTRY_END){

      /* If the entry is not empty. */
      if(ge[i].lde.LDIR_Ord != DIR_ENTRY_EMPTY){

			/* If it is a LDE. */
         if((ge[i].lde.LDIR_Attr & ATTR_LONG_NAME_MASK) == ATTR_LONG_NAME){

				/* We are not reading a LDE and we have found the last LDE. */
				if(!reading_lde && (ge[i].lde.LDIR_Ord & LAST_LONG_ENTRY)){
					reading_lde = true;
					total_lde = ge[i].lde.LDIR_Ord & 0xBF;
					lde.push_back(ge[i].lde);
					current_sum = ge[i].lde.LDIR_Chksum;

				/* We are reading a LDE and we have not found the last LDE. */
				}else if(reading_lde && !(ge[i].lde.LDIR_Ord & LAST_LONG_ENTRY) &&
					(ge[i].lde.LDIR_Ord & 0xBFU) == (total_lde - 1) &&
					current_sum == ge[i].lde.LDIR_Chksum){
					total_lde--;
					lde.push_back(ge[i].lde);

				/* We are reading a LDE and we have found the last LDE or
					we are not reading a LDE and we have not found the last LDE.*/
				}else{
					delete root_directory;
					throw FATDeviceException("The FAT file system is corrupted.");
				}

			/* If it is a DE. */
			}else{
				if((reading_lde && total_lde == 1 &&
					ComputeCheckSum(ge[i].de.DIR_Name) == current_sum) || !reading_lde){

					reading_lde = false;
					/* Replace the byte 0x05. */
					ge[i].de.DIR_Name[0] = ge[i].de.DIR_Name[0] == 0x05 ? 0xE5 : ge[i].de.DIR_Name[0];
					fat_element = FATElementFactory::CreateFATElement(&ge[i].de , lde);
					if(fat_element->IsDirectory()) ReadDirectory((FATDirectory*)fat_element);
					root_directory->InsertFATElement(fat_element);
					lde.clear();
				}else{
					throw FATDeviceException("The FAT file system is corrupted.");
				}
			}
		}else{
			if(reading_lde){
				delete root_directory;
				throw FATDeviceException("The FAT file system is corrupted.");
			}
		}


      /* Increase the counter. */
      i++;
      total_entries++;
		/* FAT32. */
		if(fat_type == FAT32){
			if(i >= (cluster_size / DIR_ENTRY_SIZE)){
				current_cluster = ReadFAT(current_cluster);
				if(IsLastCluster(current_cluster)) break;
				ReadCluster(cluster_buffer.get() , current_cluster);
				i = 0;
			}
		/* FAT12 and FAT16. */
		}else{
         if(i >= (bs_bpb.BPB_BytsPerSec / DIR_ENTRY_SIZE)){
            /* Check if is the end of root directory. */
            if(total_entries >= bs_bpb.BPB_RootEntCnt) break;
            current_sector++;
				ReadSector(cluster_buffer.get() , current_sector);
				i = 0;
         }
		}
	}
	return root_directory;
}

void FATDevice::WriteDirectory(FATDirectory* fat_directory){
	FATElement *fat_element;
	GenericEntry *ge;
	std::unique_ptr<uint8[]> cluster_buffer = std::unique_ptr<uint8[]>(new uint8[cluster_size]);
   uint32 current_cluster = 0;
	uint32 i = 0;

	current_cluster = (uint32)(fat_directory->directory_entries.back().de.DIR_FstClusHI << 16) |
		(uint32)(fat_directory->directory_entries.back().de.DIR_FstClusLO);
	if(IsLastCluster(current_cluster)) return;
	ge = (GenericEntry*) cluster_buffer.get();
	/* Avoid the replace of special entries "." and ".." .*/
	ge[i++].de = fat_directory->dot;
	ge[i++].de = fat_directory->dotdot;

	for(uint32 j = 0 ; j < fat_directory->content.size() ; j++){
		fat_element = fat_directory->content[j];
		for(uint32 k = 0 ; k < fat_element->directory_entries.size() ; k++){
			ge[i] = fat_element->directory_entries[k];

			/* Increase the counter. */
			i++;
			if(i >= (cluster_size / DIR_ENTRY_SIZE)){
				WriteCluster(cluster_buffer.get() , current_cluster);
				current_cluster = ReadFAT(current_cluster);
				i = 0;
			}
		}
	}

	while(!IsLastCluster(current_cluster)){
		memset(&(ge[i]) , 0 , DIR_ENTRY_SIZE);
		i++;
		if(i >= (cluster_size / DIR_ENTRY_SIZE)){
			WriteCluster(cluster_buffer.get() , current_cluster);
			current_cluster = ReadFAT(current_cluster);
			i = 0;
		}
	}
	delete[] cluster_buffer.release();
	for(i = 0 ; i < fat_directory->content.size() ; i++){
		fat_element = fat_directory->content[i];
		if(fat_element->IsDirectory()) WriteDirectory((FATDirectory*)fat_element);
	}
}

void FATDevice::WriteDirectoriesTree(RootDirectory* root_directory){
	FATElement *fat_element;
	GenericEntry *ge;
	std::unique_ptr<uint8[]> cluster_buffer = std::unique_ptr<uint8[]>(new uint8[cluster_size]);
	uint32 current_cluster = 0; /* Used for FAT32. */
	uint32 current_sector = 0; /* Used for FAT12 and FAT16. */
	uint32 i = 0 , total_entries = 0;

	/* FAT32. */
   if(fat_type == FAT32){
		current_cluster = bpb_fat32->BPB_RootClus;
		if(IsLastCluster(current_cluster)) return;
	/* FAT12 and FAT16. */
   }else{
		current_sector = fats_first_sector[fats_first_sector.size() - 1] + fat_size;
   }
	ge = (GenericEntry*)cluster_buffer.get();

	for(uint32 j = 0 ; j < root_directory->content.size() ; j++){
		fat_element = root_directory->content[j];
		for(uint32 k = 0 ; k < fat_element->directory_entries.size() ; k++){
			ge[i] = fat_element->directory_entries[k];

			/* Increase the counter. */
			i++;
			total_entries++;
			/* FAT32. */
			if(fat_type == FAT32){
				if(i >= (cluster_size / DIR_ENTRY_SIZE)){
					WriteCluster(cluster_buffer.get() , current_cluster);
					current_cluster = ReadFAT(current_cluster);
					i = 0;
				}
			/* FAT12 and FAT16. */
			}else{
				if(i >= (bs_bpb.BPB_BytsPerSec / DIR_ENTRY_SIZE)){
					WriteSector(cluster_buffer.get() , current_sector);
					/* Check if is the end of root directory. */
					if(total_entries >= bs_bpb.BPB_RootEntCnt) break;
					current_sector++;
					i = 0;
				}
			}
		}
	}

	/* FAT32. */
	if(fat_type == FAT32){
		while(!IsLastCluster(current_cluster)){
			memset(&(ge[i]) , 0 , DIR_ENTRY_SIZE);
			i++;
			if(i >= (cluster_size / DIR_ENTRY_SIZE)){
				WriteCluster(cluster_buffer.get() , current_cluster);
				current_cluster = ReadFAT(current_cluster);
				i = 0;
			}
		}
	/* FAT12 and FAT16. */
	}else{
		while(total_entries < bs_bpb.BPB_RootEntCnt){
			memset(&(ge[i]) , 0 , DIR_ENTRY_SIZE);
			i++;
			total_entries++;
			if(i >= (bs_bpb.BPB_BytsPerSec / DIR_ENTRY_SIZE)){
				WriteSector(cluster_buffer.get() , current_sector);
				/* Check if is the end of root directory. */
				current_sector++;
				i = 0;
			}
		}
	}
	delete[] cluster_buffer.release();
	for(i = 0 ; i < root_directory->content.size() ; i++){
		fat_element = root_directory->content[i];
		if(fat_element->IsDirectory()) WriteDirectory((FATDirectory*)fat_element);
	}
}

FATDevice::operator string(){
	stringstream buffer;
	uint32 i;

	(buffer << "\"").write((const char*)bs_fat.BS_VolLab , 11) << "\" has a ";
	switch(fat_type){
		case FAT12:
			buffer << "FAT12";
		break;
		case FAT16:
			buffer << "FAT16";
		break;
		case FAT32:
			buffer << "FAT32";
		break;
	}
	buffer << " file system." << endl << "It has " << (uint32)bs_bpb.BPB_NumFATs <<
		" FATs that has " << fat_size * (uint32)bs_bpb.BPB_BytsPerSec <<
		" bytes each or " << fat_size << " sectors." << endl <<
		"The first sector and byte offset of each FAT are respectively:" << endl;
	for(i = 0 ; i < fats_first_sector.size() ; i++){
		buffer << fats_first_sector[i] << " "
			<< fats_first_sector[i] * bs_bpb.BPB_BytsPerSec
			<< " (0x" << hex << fats_first_sector[i] * bs_bpb.BPB_BytsPerSec << dec << ")" << endl;
	}
	buffer << endl << "Root number of entries (FAT12 and FAT16 only): " <<
		bs_bpb.BPB_RootEntCnt << " that demands " <<
		sectors_root_directory << " sectors." << endl;
	buffer << "Each sector has " << bs_bpb.BPB_BytsPerSec << " bytes." << endl;
	buffer << "Each cluster has " << cluster_size << " bytes or " <<
		(uint32)bs_bpb.BPB_SecPerClus << " sectors." << endl;
	buffer << "The total number of sectors is " << total_sectors << " (0x" << hex <<
		total_sectors << dec <<	")." << endl;
	buffer << "The total number of clusters is " << total_clusters << " (0x" << hex <<
		total_clusters << dec <<	")." << endl;

	buffer << "The first data sector is " << first_data_sector << " (0x" << hex <<
		first_data_sector << dec <<	")." << endl;
	return buffer.str();
}

ostream &operator<<(ostream &stream , FATDevice &fat_device){
	return (stream << (string)fat_device);
}
