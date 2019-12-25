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

#include "types.h"
#include "unicode.h"

#include <vector>
using namespace std;

void ConvertToUTF8(uint32 code , vector<uint8> &text_utf8){
	/* Now convert to UTF8. */
	if(code < 0x80){
		text_utf8.push_back(uint8(code));
	}else if(code < 0x800){
		text_utf8.push_back(uint8(0xC0 | (code >> 6)));
		text_utf8.push_back(uint8(0x80 | (code & 0x3F)));
	}else if(code < 0x10000){
		text_utf8.push_back(uint8(0xE0 | (code >> 12)));
		text_utf8.push_back(uint8(0x80 | ((code >> 6) & 0x3F)));
		text_utf8.push_back(uint8(0x80 | (code & 0x3F)));
	}else{
		text_utf8.push_back(uint8(0xF0 | (code >> 18)));
		text_utf8.push_back(uint8(0x80 | ((code >> 12) & 0x3F)));
		text_utf8.push_back(uint8(0x80 | ((code >> 6) & 0x3F)));
		text_utf8.push_back(uint8(0x80 | (code & 0x3F)));
	}
}

char* Unicode::ConvertFromByteToUTF8(const char* text){
	vector<uint8> text_utf8;
	vector<uint8> text_byte;
	uint32 i;
	
	for(i = 0; text[i]; i++) {
		text_byte.push_back(uint8(text[i]));
	}
	text_byte.push_back(0);
	
	text_utf8 = ConvertFromByteToUTF8(text_byte);
	
	uint8* text_to_return = new uint8[text_utf8.size()];
	for(i = 0; i < text_utf8.size(); i++){
		text_to_return[i] = text_utf8[i];
	}

	return (char*) text_to_return;
}

vector<uint8> Unicode::ConvertFromByteToUTF8(vector<uint8> text_byte){
	vector<uint8> text_utf8;
	uint32 i;

	for(i = 0 ; i < text_byte.size() ; i++){
		ConvertToUTF8(text_byte[i] , text_utf8);
	}
	return text_utf8;
}

char* Unicode::ConvertFromUTF16ToUTF8(const wchar_t* text){
	vector<uint16> text_utf16;
	vector<uint8> text_utf8;
	uint32 i;
	
	for(i = 0; text[i]; i++) {
		text_utf16.push_back(wchar_t(text[i]));
	}
	text_utf16.push_back(0);
	
	text_utf8 = ConvertFromUTF16ToUTF8(text_utf16);
	
	uint8* text_to_return = new uint8[text_utf8.size()];
	for(i = 0; i < text_utf8.size(); i++){
		text_to_return[i] = text_utf8[i];
	}

	return (char*) text_to_return;
}

vector<uint8> Unicode::ConvertFromUTF16ToUTF8(vector<uint16> text_utf16){
	vector<uint8> text_utf8;
	uint16 c;
	uint32 i , code;

	for(i = 0 ; i < text_utf16.size() ; i++){
		c = text_utf16[i];
		if(c < 0xD800 || c > 0xDFFF){
			code = c;
		}else{
			code = 0x10000 + ((c & 0x3FF) << 10);//XXX: There are more error cases.
			if(++i >= text_utf16.size()){
				throw UnicodeException();
			}
			c = text_utf16[++i];
			code |= (c & 0x3FF);
		}
		ConvertToUTF8(code , text_utf8);
	}
	return text_utf8;
}
