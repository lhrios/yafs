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
 * Exception Module: defines an exception class.
 */

#ifndef YAFS_EXCEPTION_H
	#define YAFS_EXCEPTION_H

	#include <iostream>
	#include <string>

	using namespace std;

	class Exception {
		public:
			Exception(string message){
				exception_message = message;
			}

			operator string(){
				return exception_message;
			}

			friend ostream &operator<<(ostream &stream , Exception &e);

		private:
			string exception_message;
	};

	inline ostream &operator<<(ostream &stream , Exception &e){
		return(stream << e.exception_message);
	}

#endif
