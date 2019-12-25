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
 * Xercesc Module: groups some functions related with the Xercesc library.
 */

#ifndef __XERCESC_H
#define __XERCESC_H

	#include "exception.h"
	#include "types.h"
	/* Xerces includes: */
	#include <xercesc/util/TransService.hpp>
	XERCES_CPP_NAMESPACE_USE

	class Xercesc {
		public:
			static void Initialize();
			static void Terminate();
			static uint8* TranscodeToUTF8(const XMLCh *string);
			static XMLCh* TranscodeFromUTF8(const uint8 *string);

			class XercescException : public Exception {
				public:
					XercescException(string message = ""):Exception(message){}
			};
		private:
			static XMLTranscoder *xml_transcoder_utf8;
			static bool xerces_was_initialized;
	};


#endif
