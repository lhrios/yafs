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
 * FAT Elements Module: represents the FAT file system elements.
 */

#ifndef YAFS_FAT_ELEMENTS_H
	#define YAFS_FAT_ELEMENTS_H

	#include "fat.h"
	#include "fat_device_type.h"
	#include "exception.h"
	#include "string_compare.h"

	#include <map>
	#include <string>
	#include <vector>
	/* Xerces includes: */
	#include <xercesc/dom/DOM.hpp>
	using namespace std;
	XERCES_CPP_NAMESPACE_USE

	class InvalidFATElementException : public Exception {
		public:
			InvalidFATElementException(string message = ""):Exception(message){}
	};

	class FATDirectory;
	class FATElementFactory;
	class FATFile;
	class RootDirectory;

	class FATElement {
		public:
			FATElement(const DirectoryEntryStructure *de ,
				const vector<LongDirectoryEntryStructure> lde);
			virtual ~FATElement(){
				if(long_name) delete[] long_name;
				delete[] short_name;
			}

			virtual bool IsDirectory() = 0;
			virtual string ToXML(uint32 n_tabs) = 0;
			bool operator<(FATElement &fat_element){
				return order < fat_element.order;
			}
			bool HasVolumeIDAttribute(){
				return (attributes & ATTR_VOLUME_ID) != 0;
			}
			friend class FATDevice;
			friend class FATDirectory;
			friend class RootDirectory;
		protected:
			uint8 *short_name;
			uint8 *long_name;
			uint32 order;
			bool reordered;
			uint8 attributes;

			vector<GenericEntry> directory_entries;

			static uint8 *GetShortName(const DirectoryEntryStructure *de);
	};

	class FATFile : public FATElement {
		public:
			FATFile(const DirectoryEntryStructure *de ,
				const vector<LongDirectoryEntryStructure> lde):FATElement(de , lde){
			}
			virtual bool IsDirectory(){
				return false;
			}
			virtual string ToXML(uint32 n_tabs);
	};

	class FATDirectory : public FATElement {
		public:
			FATDirectory(const DirectoryEntryStructure *de ,
				const vector<LongDirectoryEntryStructure> lde):FATElement(de , lde){
			}
			virtual ~FATDirectory();
			virtual bool IsDirectory(){
				return true;
			}
			virtual string ToXML(uint32 n_tabs);
			void InsertFATElement(FATElement *fat_element);
			void Sort();
			friend class FATDevice;
			friend class RootDirectory;
		private:
			DirectoryEntryStructure dot, dotdot;
			vector<FATElement*> content;
			map<const char* , FATElement* , StringCompare> content_map;

			bool ReorderFATElement(uint8* short_name , uint32 order , FATElement** fat_element);
			void ReorderFATDirectory(DOMElement* directory_element);
	};

	class FATElementFactory {
		public:
			static FATElement* CreateFATElement(const DirectoryEntryStructure *de ,
				const vector<LongDirectoryEntryStructure> lde);
	};

	class RootDirectory {
		public:
			~RootDirectory();
			void InsertFATElement(FATElement *fat_element);
			string ToXML();
			void ImportNewOrder(const char* xml_file);

			class RootDirectoryException : public Exception {
				public:
					RootDirectoryException(string message = ""):Exception(message){}
			};
			friend class FATDevice;
		private:
			vector<FATElement*> content;
			map<const char* , FATElement* , StringCompare> content_map;

			void Sort();
			bool ReorderFATElement(uint8* short_name , uint32 order, FATElement** fat_element);
	};

#endif
