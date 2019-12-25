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

#include <iostream>
#include <fstream>
#include "command_line_parser.h"
#include "exception.h"
#include "fat_device.h"

#include <cassert>
#include <cstring>

using namespace std;

enum OperationMode {
	READ_DIRECTORIES_TREE,
	WRITE_DIRECTORIES_TREE,
	FETCH_DEVICE_INFORMATION,
	INVALID_MODE
};

void PrintHelp(){
	cerr <<
		"Usage: yafs -d device_path -f file_path -{r | w} [-v]" << endl <<
		"       yafs -d device_path -i [-v]" << endl << endl <<
		"-v   Activates the verbose mode that will print debug information." << endl <<
		"-d   It is used to specify the device that has a FAT file system. On Windows," << endl <<
		"     the argument must be the device letter, i.e. \"e:\". On Unix, the" << endl <<
		"     argument is the device file, i.e. \"/dev/hdb1\"." << endl << endl <<
		"-f   It is used to specify the input or output file. If the option -r is used," << endl <<
		"     the program will use this file to store the current file system directory" << endl <<
		"     tree. If the option -w is used, the program will read the sorted file" << endl <<
		"     system directory tree from the file." << endl << endl <<
		"-i   With this option, the program only prints some information about the" << endl <<
		"     device file system. It can't be combined with the -r or -w options." << endl << endl <<
		"-r   With this option the program will print the current file system directory" << endl <<
		"     tree in the file specified with -f option. It can't be combined with the" << endl <<
		"     -i or -w options." << endl << endl <<
		"-w   This option is the only one that modifies the device file system. The" << endl <<
		"     program will read the sorted file system directory tree from the file" << endl <<
		"     specified with -f option and then it will change the device file system" << endl <<
		"     so the files and directories on it have an order equal to the order" << endl <<
		"     specified in the input file. It can't be combined with the -i or -r" << endl <<
		"     options." << endl;
}

void PrintErrorMessage(){
	cerr << "Invalid program call. Try \"yafs -h\" to see the help." << endl;
}

#include "utils.h"

int main(int argc , char **argv){
	char *device_path = NULL, *io_file_path = NULL;
	OperationMode operation_mode = INVALID_MODE;

	/* Parse the command line. */
	{
		CommandLineParser commandLineParser(argc , argv , string("d:?f:?r?w?i?h?v?").c_str());

		if (commandLineParser.isValid()) {
			int exclusive_options_count = 0;
			const CommandLineParser::CommandLineOption *option = NULL;

			if ((option = commandLineParser.getOption('r'))->found) {
				exclusive_options_count++;
				operation_mode = READ_DIRECTORIES_TREE;
			}
			if ((option = commandLineParser.getOption('w'))->found) {
				exclusive_options_count++;
				operation_mode = WRITE_DIRECTORIES_TREE;
			}
			if ((option = commandLineParser.getOption('i'))->found) {
				exclusive_options_count++;
				operation_mode = FETCH_DEVICE_INFORMATION;
			}
			if ((option = commandLineParser.getOption('h'))->found) {
				exclusive_options_count++;
			}

			if (exclusive_options_count > 1
					|| exclusive_options_count == 0){
				PrintErrorMessage();
				return 1;
			}

			if (exclusive_options_count == 1 && operation_mode == INVALID_MODE) {
				PrintHelp();
				return 0;
			}

			if ((option = commandLineParser.getOption('d'))->found) {
				device_path = option->argument_value;
			}

			if ((option = commandLineParser.getOption('v'))->found) {
				LogUtils::SetEnabled(true);
			}

			if ((option = commandLineParser.getOption('f'))->found) {
				io_file_path = option->argument_value;
			}

			assert (operation_mode != INVALID_MODE);
			if (device_path == NULL
					|| (io_file_path == NULL && operation_mode != FETCH_DEVICE_INFORMATION)) {
				PrintErrorMessage();
				return 1;
			}

		} else {
			PrintErrorMessage();
			return 1;
		}
	}

	FATDevice *fat_device = NULL;
   try{
		char *final_device_path;
		RootDirectory *root_directory;

		#ifdef WIN_SYSTEM
			if(strlen(device_path) != 2 || device_path[1] != ':'){
				PrintErrorMessage();
				return 1;
			}
			final_device_path = new char[7];
			memcpy(final_device_path , "\\\\.\\?:" , 7);
			final_device_path[4] = device_path[0];
		#elif UNIX_SYSTEM
			final_device_path = device_path;
		#endif

		switch(operation_mode){
			case READ_DIRECTORIES_TREE:{
				fat_device = new FATDevice(final_device_path, "r");
				ofstream io_file(io_file_path);
				if(!io_file.is_open()){
					cerr << "The file \"" << io_file_path << "\" could not be opened." << endl;
					return 1;
				}
				root_directory = fat_device->ReadDirectoriesTree();
				io_file << root_directory->ToXML();
				delete root_directory;
			}break;
			case WRITE_DIRECTORIES_TREE:{
				fat_device = new FATDevice(final_device_path, "r+");
				root_directory = fat_device->ReadDirectoriesTree();
				root_directory->ImportNewOrder(io_file_path);
				fat_device->WriteDirectoriesTree(root_directory);
				delete root_directory;
			}break;
			case FETCH_DEVICE_INFORMATION:{
				fat_device = new FATDevice(final_device_path, "r");
				cout << *fat_device;
			}break;
			case INVALID_MODE:
			break;
		}
   }catch(Exception e){
     cerr << "Exception: " << e << endl;
	  return 1;
   }

   if (fat_device != NULL) {
		delete fat_device;
	}

   return 0;
}
