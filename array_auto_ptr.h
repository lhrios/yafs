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
 * Array Auto Pointer Module: implements a very simple auto_ptr for arrays.
 */

#ifndef YAFS_ARRAY_AUTO_PTR_H
	#define YAFS_ARRAY_AUTO_PTR_H

	#include <cstdlib>

	template<class T> class array_auto_ptr{
		public:
			array_auto_ptr(T* new_array_pointer){
				array_pointer = new_array_pointer;
			}
			~array_auto_ptr(){
				if(array_pointer != NULL) delete[] array_pointer;
			}
			T* get(){
				return array_pointer;
			}
			T* release(){
				T* aux = array_pointer;
				array_pointer = NULL;
				return aux;
			}
		private:
			T *array_pointer;
	};

#endif
