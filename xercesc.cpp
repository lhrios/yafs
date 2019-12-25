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

#include "xercesc.h"

#include <cassert>
#include <string>
/* Xerces includes: */
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/util/TransService.hpp>

using namespace std;
XERCES_CPP_NAMESPACE_USE

bool Xercesc::xerces_was_initialized = false;
XMLTranscoder *Xercesc::xml_transcoder_utf8 = NULL;

void Xercesc::InitializeXercesc(){
	if(xerces_was_initialized) return;
	try{
		XMLTransService::Codes res_value;

		XMLPlatformUtils::Initialize();
		xml_transcoder_utf8 = XMLPlatformUtils::fgTransService->makeNewTranscoderFor("UTF8" ,
			res_value , 64);
		xerces_was_initialized = true;
	}catch(XMLException &xml_exception){
		char *message_buffer = XMLString::transcode(xml_exception.getMessage());
		string message(message_buffer);
		XMLString::release(&message_buffer);
		throw XercescException(message);
	}
}

void Xercesc::TerminateXercesc(){
	assert(xerces_was_initialized);
	try{
		delete xml_transcoder_utf8;
		XMLPlatformUtils::Terminate();
		xerces_was_initialized = false;
	}catch(XMLException &xml_exception){
		char *message_buffer = XMLString::transcode(xml_exception.getMessage());
		string message(message_buffer);
		XMLString::release(&message_buffer);
		throw XercescException(message);
	}
}

uint8* Xercesc::TranscodeToUTF8(const XMLCh *string){
	const uint32 buffer_length = 4096;
	uint8* string_utf8 , buffer[buffer_length];
	uint32 size , characters_processed , length;

	length = XMLString::stringLen(string);
	size = xml_transcoder_utf8->transcodeTo(string , length , (XMLByte*)buffer ,
		buffer_length , characters_processed , XMLTranscoder::UnRep_Throw);
	assert(characters_processed <= length);
	string_utf8 = new uint8[size + 1];
	memcpy(string_utf8 , buffer , size);
	string_utf8[size] = '\0';

	return string_utf8;
}
