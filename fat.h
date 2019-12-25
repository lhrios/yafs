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
 * FAT Modules: defines FAT file system structures.
 */

#ifndef YAFS_FAT_H
	#define YAFS_FAT_H

	#include "pack.h"
	#include "types.h"

	#define LAST_LONG_ENTRY 0x40
	#define DIR_ENTRY_END 0x00
	#define DIR_ENTRY_EMPTY 0xE5

	#define DIR_ENTRY_SIZE 32

	#define ATTR_READ_ONLY 0x01
	#define ATTR_HIDDEN 0x02
	#define ATTR_SYSTEM 0x04
	#define ATTR_VOLUME_ID 0x08
	#define ATTR_DIRECTORY 0x10
	#define ATTR_ARCHIVE 0x20
	#define ATTR_LONG_NAME (ATTR_READ_ONLY | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_VOLUME_ID)
	#define ATTR_LONG_NAME_MASK (ATTR_READ_ONLY | ATTR_HIDDEN | ATTR_SYSTEM |\
		ATTR_VOLUME_ID | ATTR_DIRECTORY | ATTR_ARCHIVE )

	/* The Boot Sector and the BIOS Parameter Block are present in all FAT systems. */
	PACK(struct BootSectorBIOSParameterBlock{
		uint8 BS_jmpBoot[3];
		uint8 BS_OEMName[8];
		uint16 BPB_BytsPerSec;
		uint8 BPB_SecPerClus;
		uint16 BPB_RsvdSecCnt;
		uint8 BPB_NumFATs;
		uint16 BPB_RootEntCnt;
		uint16 BPB_TotSec16;
		uint8 BPB_Media;
		uint16 BPB_FATSz16;
		uint16 BPB_SecPerTrk;
		uint16 BPB_NumHeads;
		uint32 BPB_HiddSec;
		uint32 BPB_TotSec32;
	});

	/* BIOS Parameter Block is only present in FAT32. */
	PACK(struct BIOSParameterBlockFAT32{
		uint32 BPB_FATSz32;
		uint16 BPB_ExtFlags;
		uint16 BPB_FSVer;
		uint32 BPB_RootClus;
		uint16 BPB_FSInfo;
		uint16 BPB_BkBootSec;
		uint8 BPB_Reserved[12];
	});

	/* The Boot Sector is present in all FAT systems. */
	PACK(struct BootSectorFAT{
		uint8 BS_DrvNum;
		uint8 BS_Reserved1;
		uint8 BS_BootSig;
		uint32 BS_VolID;
		uint8 BS_VolLab[11];
		uint8 BS_FilSysType[8];
	});

	/* Directory Entry Structure. */
	PACK(struct DirectoryEntryStructure{
		uint8 DIR_Name[11];
		uint8 DIR_Attr;
		uint8 DIR_NTRes;
		uint8 DIR_CrtTimeTenth;
		uint16 DIR_CrtTime;
		uint16 DIR_CrtDate;
		uint16 DIR_LstAccDate;
		uint16 DIR_FstClusHI;
		uint16 DIR_WrtTime;
		uint16 DIR_WrtDate;
		uint16 DIR_FstClusLO;
		uint32 DIR_FileSize;
	});

	/* Long Directory Entry Structure. */
	PACK(struct LongDirectoryEntryStructure{
		uint8 LDIR_Ord;
		uint16 LDIR_Name1[5];
		uint8 LDIR_Attr;
		uint8 LDIR_Type;
		uint8 LDIR_Chksum;
		uint16 LDIR_Name2[6];
		uint16 LDIR_FstClusLO;
		uint16 LDIR_Name3[2];
	});

	/* The union of a LDES with a DES. */
	PACK(union GenericEntry{
		DirectoryEntryStructure  de;
		LongDirectoryEntryStructure lde;
	});

#endif
