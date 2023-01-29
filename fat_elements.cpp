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

#include "fat.h"
#include "fat_elements.h"
#include "types.h"
#include "unicode.h"
#include "utils.h"
#include "xercesc.h"

#include <algorithm>
#include <memory>
#include <sstream>
#include <string>

/* Xerces includes: */
#include <xercesc/dom/DOMException.hpp>
#include <xercesc/dom/DOM.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/sax/HandlerBase.hpp>
#include <xercesc/validators/common/Grammar.hpp>
using namespace std;
XERCES_CPP_NAMESPACE_USE

const string xsd_file_name("fat_file_system_tree.xsd");

stringstream* PrintTabs(stringstream *buffer , uint32 n_tabs){
	for( ; n_tabs > 0 ; n_tabs--)
		(*buffer) << '\t';
	return buffer;
}

template <class CharSequence> stringstream* PrintAndReplaceReservedCharacters(stringstream *buffer , CharSequence char_sequence){
	for(uint32 i = 0 ; char_sequence[i] ; i++) {
		switch (char_sequence[i]) {
			case '<':
				(*buffer) << "&lt;";
			break;
			case '>':
				(*buffer) << "&gt;";
			break;
			case '&':
				(*buffer) << "&amp;";
			break;
			case '\'':
				(*buffer) << "&apos;";
			break;
			case '"':
				(*buffer) << "&quot;";
			break;
			default:
				(*buffer) << ((char) char_sequence[i]);
			break;
		}
	}
	return buffer;
}

/* FATElementFactory. */
FATElement* FATElementFactory::CreateFATElement(const DirectoryEntryStructure *de ,
	const vector<LongDirectoryEntryStructure> lde){

	if(!(de->DIR_Attr & (ATTR_DIRECTORY | ATTR_VOLUME_ID))){
		return new FATFile(de , lde);
	}else if((de->DIR_Attr & (ATTR_DIRECTORY | ATTR_VOLUME_ID)) == ATTR_DIRECTORY){
		return new FATDirectory(de , lde);
   }else if((de->DIR_Attr & (ATTR_DIRECTORY | ATTR_VOLUME_ID)) == ATTR_VOLUME_ID){
		return new FATFile(de , lde);
	}else{
		throw InvalidFATElementException("The file system has an invalid entry.");
	}
}

/* FATElement. */
// TODO: Move to static method on class FATElement?
bool FATElementCompare(FATElement* a , FATElement* b){
	return *a < *b;
}

void InsertUTF16Char(uint16 c , vector<uint16> &long_name_utf16){
	/* Some characters must be represented by the proper entity. */
	switch(c){
		case '"':
			long_name_utf16.push_back('&');
			long_name_utf16.push_back('q');
			long_name_utf16.push_back('u');
			long_name_utf16.push_back('o');
			long_name_utf16.push_back('t');
			long_name_utf16.push_back(';');
		break;
		case '&':
			long_name_utf16.push_back('&');
			long_name_utf16.push_back('a');
			long_name_utf16.push_back('m');
			long_name_utf16.push_back('p');
			long_name_utf16.push_back(';');
		break;
		case '\'':
			long_name_utf16.push_back('&');
			long_name_utf16.push_back('a');
			long_name_utf16.push_back('p');
			long_name_utf16.push_back('o');
			long_name_utf16.push_back('s');
			long_name_utf16.push_back(';');
		break;
		case '<':
			long_name_utf16.push_back('&');
			long_name_utf16.push_back('l');
			long_name_utf16.push_back('t');
			long_name_utf16.push_back(';');
		break;
		case '>':
			long_name_utf16.push_back('&');
			long_name_utf16.push_back('g');
			long_name_utf16.push_back('t');
			long_name_utf16.push_back(';');
		break;
		default:
			long_name_utf16.push_back(c);
		break;
	}
}

FATElement::FATElement(const DirectoryEntryStructure *de ,
	const vector<LongDirectoryEntryStructure> lde){
	GenericEntry ge;

	if(lde.size() > 0){
		int i, j;
		size_t long_name_length;
		vector<uint16> long_name_utf16;
		vector<uint8> long_name_utf8;

		for(i = (int) lde.size() - 1 ; i >= 0 ; i--){
			for(j = 0 ; j < 5 ; j++)
				InsertUTF16Char(lde[i].LDIR_Name1[j] , long_name_utf16);
			for(j = 0 ; j < 6 ; j++)
				InsertUTF16Char(lde[i].LDIR_Name2[j] , long_name_utf16);
			for(j = 0 ; j < 2 ; j++)
				InsertUTF16Char(lde[i].LDIR_Name3[j] , long_name_utf16);
			ge.lde = lde[i];
			directory_entries.insert(directory_entries.begin() , ge);
		}

		/* Remove the padding characters. */
      for(i = 0 ; i < (int)long_name_utf16.size() ; i++){
         if(long_name_utf16[i] == 0 && long_name_utf16.size() - i + 1 > 0){
            long_name_utf16.resize(i + 1 , 0);
            break;
         }
      }
		try{
			long_name_utf8 = Unicode::ConvertFromUTF16ToUTF8(long_name_utf16);
		}catch(Unicode::UnicodeException unicode_exception){
			throw InvalidFATElementException("The file system has an invalid entry.");
		}
		long_name_length = long_name_utf8[long_name_utf8.size() - 1] == 0 ?
			long_name_utf8.size() : long_name_utf8.size() + 1;
		long_name = new uint8[long_name_length];
      for(i = 0 ; i < (int)(long_name_length - 1) ; i++)
			long_name[i] = long_name_utf8[i];
		long_name[i] = '\0';
	}else{
		long_name = NULL;
	}
	ge.de = *de;
	directory_entries.push_back(ge);
	short_name = GetShortName(de);
	order = 0;
	reordered = false;
	attributes = de->DIR_Attr;
}

uint8* FATElement::GetShortName(const DirectoryEntryStructure *de){
	uint32 i = 0;
	uint8 *final_short_name;
	vector<uint8> short_name_byte , short_name_utf8;
	bool has_extension = false;

	while(i < 8 && de->DIR_Name[i] != ' ')
		short_name_byte.push_back(de->DIR_Name[i++]);

	short_name_byte.push_back('.');
	i = 8;
	while(i < 11 && de->DIR_Name[i] != ' '){
		short_name_byte.push_back(de->DIR_Name[i++]);
		has_extension = true;
	}
	if(!has_extension) short_name_byte.pop_back();

	short_name_utf8 = Unicode::ConvertFromByteToUTF8(short_name_byte);
	final_short_name = new uint8[short_name_utf8.size() + 1];
	for(i = 0 ; i < short_name_utf8.size() ; i++)
		final_short_name[i] = short_name_utf8[i];
	final_short_name[i] = '\0';
	return final_short_name;
}

/* FATFile. */
string FATFile::ToXML(uint32 n_tabs){
	stringstream buffer;

	(*PrintTabs(&buffer , n_tabs)) << "<file order=\"" << order << "\"";
	if (HasVolumeIDAttribute()) {
		buffer << " volume=\"true\"";
	}
	buffer << ">" << endl;

	if(long_name)
		(*PrintTabs(&buffer , n_tabs + 1)) << "<long_name>" << long_name << "</long_name>" << endl;
	(*PrintTabs(&buffer , n_tabs + 1)) << "<short_name>";
	(*PrintAndReplaceReservedCharacters(&buffer, short_name)) << "</short_name>" << endl;
	(*PrintTabs(&buffer , n_tabs)) << "</file>" << endl;

	return buffer.str();
}

/* FATDirectory. */
void ThrowDoNotMatchException(string message = string("")){
	throw RootDirectory::RootDirectoryException(
		string("The device file system do not match with the "
			"file system in input file.").append(message));
}

FATDirectory::~FATDirectory(){
	uint32 i;
	for(i = 0 ; i < content.size() ; i++)
      delete content[i];
}

string FATDirectory::ToXML(uint32 n_tabs){
	stringstream buffer;
	uint32 i;

	(*PrintTabs(&buffer , n_tabs)) << "<directory order=\"" << order << "\">" << endl;
	if(long_name)
		(*PrintTabs(&buffer , n_tabs + 1)) << "<long_name>" << long_name << "</long_name>" << endl;
	(*PrintTabs(&buffer , n_tabs + 1)) << "<short_name>";
	(*PrintAndReplaceReservedCharacters(&buffer, short_name)) << "</short_name>" << endl;
	for(i = 0 ; i < content.size() ; i++){
		buffer << content[i]->ToXML(n_tabs + 1);
	}
	(*PrintTabs(&buffer , n_tabs)) << "</directory>" << endl;

	return buffer.str();
}

void FATDirectory::InsertFATElement(FATElement *fat_element){
	fat_element->order = (uint32) (content.size() + 1) * 100;
	content.push_back(fat_element);
	content_map.insert(make_pair((const char*)fat_element->short_name , fat_element));
}

void FATDirectory::Sort(){
	sort(content.begin() , content.end() , FATElementCompare);
	for(uint32 i = 0 ; i < content.size() ; i++)
		if(content[i]->IsDirectory()) ((FATDirectory*)content[i])->Sort();
}

bool FATDirectory::ReorderFATElement(uint8* short_name , uint32 order , FATElement** fat_element){
	map<const char* , FATElement* , StringCompare>::iterator i;

	i = content_map.find((char*)short_name);
	if(i == content_map.end() || i->second->reordered)
		return false;
	i->second->reordered = true;
	i->second->order = order;
	*fat_element = i->second;
	return true;
}

/* RootDirectory. */
string RootDirectory::ToXML(){
	stringstream buffer;
	uint32 i;

	buffer << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << endl;
	buffer << "<root xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:noNamespaceSchemaLocation=\"";
	(*PrintAndReplaceReservedCharacters(&buffer, ExecutableDirectoryUtils::GetExecutableDirectoryURIUFT8())) << xsd_file_name << "\">" << endl;

	for(i = 0 ; i < content.size() ; i++){
		buffer << content[i]->ToXML(1);
	}
	buffer << "</root>" << endl;

	return buffer.str();
}

class DOMTreeErrorReporter : public ErrorHandler {
	public:
		DOMTreeErrorReporter(){
			resetErrors();
		}

		void warning(const SAXParseException& toCatch);
		void error(const SAXParseException& toCatch);
		void fatalError(const SAXParseException& toCatch);
		void resetErrors(){
			saw_errors = false;
			errors_buffer.str("");
		}
		bool SawErrors(){
			return saw_errors;
		}
		string GetErrors(){
			return errors_buffer.str();
		}
	private:
		bool saw_errors;
		stringstream errors_buffer;
};

void DOMTreeErrorReporter::warning(const SAXParseException &toCatch){
	char *message = XMLString::transcode(toCatch.getMessage());
	cerr << "Warning at line " << toCatch.getLineNumber() << ": " <<
		message << "." << endl;
	XMLString::release(&message);
}

void DOMTreeErrorReporter::error(const SAXParseException &toCatch){
	char *message = XMLString::transcode(toCatch.getMessage());
	saw_errors = true;
	errors_buffer << "Error at line " << toCatch.getLineNumber() << ": " <<
		message << "." << endl;
	XMLString::release(&message);
}

void DOMTreeErrorReporter::fatalError(const SAXParseException &toCatch){
	error(toCatch);
}

const XMLCh file_utf16_str[] = {
	chLatin_f ,
	chLatin_i ,
	chLatin_l ,
	chLatin_e ,
	0x0
};
const XMLCh directory_utf16_str[] = {
	chLatin_d ,
	chLatin_i ,
	chLatin_r ,
	chLatin_e ,
	chLatin_c ,
	chLatin_t ,
	chLatin_o ,
	chLatin_r ,
	chLatin_y ,
	0x0
};
const XMLCh order_utf16_str[] = {
	chLatin_o ,
	chLatin_r ,
	chLatin_d ,
	chLatin_e ,
	chLatin_r ,
	0x0
};
const XMLCh short_name_utf16_str[] = {
	chLatin_s ,
	chLatin_h ,
	chLatin_o ,
	chLatin_r ,
	chLatin_t ,
	chUnderscore ,
	chLatin_n ,
	chLatin_a ,
	chLatin_m ,
	chLatin_e ,
	0x0
};

RootDirectory::~RootDirectory(){
	uint32 i;
	for(i = 0 ; i < content.size() ; i++)
      delete content[i];
}

void RootDirectory::InsertFATElement(FATElement *fat_element){
	fat_element->order = (uint32) (content.size() + 1) * 100;
	content.push_back(fat_element);
	content_map.insert(make_pair((const char*)fat_element->short_name , fat_element));
}

uint8* ExtractShortName(DOMElement* element){
	DOMNodeList *children = element->getChildNodes();
	DOMNode *short_name_element;

	for(uint32 i = 0 ; i < children->getLength() ; i++){
		short_name_element = children->item(i);
		if(short_name_element->getNodeType() != DOMNode::ELEMENT_NODE) continue;
		if(XMLString::equals(short_name_utf16_str , short_name_element->getNodeName())){
			DOMNodeList *children = short_name_element->getChildNodes();
			/* It must have only one child. */
			if(children->getLength() != 1 ||
				children->item(0)->getNodeType() != DOMNode::TEXT_NODE) continue;
				return Xercesc::TranscodeToUTF8(children->item(0)->getNodeValue());
		}
	}
	return NULL;
}

bool RootDirectory::ReorderFATElement(uint8* short_name , uint32 order , FATElement** fat_element){
	map<const char* , FATElement* , StringCompare>::iterator i;

	i = content_map.find((char* )short_name);
	if(i == content_map.end() || i->second->reordered)
		return false;
	i->second->reordered = true;
	i->second->order = order;
	*fat_element = i->second;
	return true;
}

void FATDirectory::ReorderFATDirectory(DOMElement* directory_element){
	DOMNode *child;
	DOMNodeList *children;
	uint32 children_reordered = 0 , order;
	uint8 *short_name;
	bool is_directory = false;
	FATElement *fat_element;

	children = directory_element->getChildNodes();
	for(uint32 i = 0 ; i < children->getLength() ; i++){
		child = children->item(i);
		if(child->getNodeType() != DOMNode::ELEMENT_NODE) continue;
		is_directory = false;
		if(XMLString::equals(file_utf16_str , child->getNodeName()) ||
			(is_directory = XMLString::equals(directory_utf16_str , child->getNodeName()))){
			/* The Validation guarantees that order attribute will always exist. */
			XMLString::textToBin(((DOMElement*)child)->getAttribute(order_utf16_str) , order);
			short_name = ExtractShortName((DOMElement*)child);
			if(!short_name) ThrowDoNotMatchException();
			if(!ReorderFATElement(short_name , order , &fat_element)){
				string short_name_str((char*)short_name);
				delete[] short_name;
				ThrowDoNotMatchException(short_name_str);
			}
			if(is_directory){
				if(!fat_element->IsDirectory()) ThrowDoNotMatchException();
				/* XXX: When an exception is thrown is possible that some memory leaks. */
				/* To solve this call must be placed after the loop (in another loop). */
				((FATDirectory*)fat_element)->ReorderFATDirectory((DOMElement*)child);
			}
			children_reordered++;
			delete[] short_name;
		}
	}
	if(children_reordered != content.size())
		ThrowDoNotMatchException();
}

void RootDirectory::ImportNewOrder(const char* xml_file){
	Xercesc::Initialize();

	try{
		std::unique_ptr<XercesDOMParser> parser = std::unique_ptr<XercesDOMParser>(new XercesDOMParser());
		DOMTreeErrorReporter dom_tree_error_reporter;
		DOMElement *root;
		DOMNode *child;
		DOMNodeList *children;
		uint32 children_reordered = 0 , order;
		uint8 *short_name;
		bool is_directory = false;
		FATElement *fat_element;

		parser->setValidationScheme(XercesDOMParser::Val_Always);
		parser->setDoNamespaces(true);
		parser->setDoSchema(true);
		parser->setValidationSchemaFullChecking(true);
		parser->setValidationConstraintFatal(true);
		parser->setExternalNoNamespaceSchemaLocation((ExecutableDirectoryUtils::GetExecutableDirectoryNativeEncoding() + xsd_file_name).c_str());
		parser->setIncludeIgnorableWhitespace(false);
		parser->setCreateCommentNodes(false);

		parser->setErrorHandler(&dom_tree_error_reporter);
		parser->parse(xml_file);
		if(dom_tree_error_reporter.SawErrors()){
			cerr << dom_tree_error_reporter.GetErrors() << endl;
			throw RootDirectoryException(dom_tree_error_reporter.GetErrors());
		}

		root = parser->getDocument()->getDocumentElement();
		children = root->getChildNodes();
		for(uint32 i = 0 ; i < children->getLength() ; i++){
			child = children->item(i);
			if(child->getNodeType() != DOMNode::ELEMENT_NODE) continue;
			is_directory = false;
			if(XMLString::equals(file_utf16_str , child->getNodeName()) ||
				(is_directory = XMLString::equals(directory_utf16_str , child->getNodeName()))){
				/* The Validation guarantees that order attribute will always exist. */
				XMLString::textToBin(((DOMElement*)child)->getAttribute(order_utf16_str) , order);
				short_name = ExtractShortName((DOMElement*)child);
				if(!short_name) ThrowDoNotMatchException();
				if(!ReorderFATElement(short_name , order , &fat_element)){
					string short_name_str((char*)short_name);
					delete[] short_name;
					ThrowDoNotMatchException(short_name_str);
				}
				if(is_directory){
					if(!fat_element->IsDirectory()) ThrowDoNotMatchException();
					/* XXX: When an exception is thrown is possible that some memory leaks. */
					/* To solve this call must be placed after the loop (in another loop). */
					((FATDirectory*)fat_element)->ReorderFATDirectory((DOMElement*)child);
				}
				children_reordered++;
				delete[] short_name;
			}
		}
		if(children_reordered != content.size())
			ThrowDoNotMatchException();
		Sort();

	}catch(DOMException &dom_exception){
		char *message_buffer = XMLString::transcode(dom_exception.getMessage());
		string message(message_buffer);
		XMLString::release(&message_buffer);
		Xercesc::Terminate();
		throw RootDirectoryException(message);
	}catch(RootDirectoryException root_directory_exception){
		Xercesc::Terminate();
		throw root_directory_exception;
	}

	Xercesc::Terminate();
}

void RootDirectory::Sort(){
	sort(content.begin() , content.end() , FATElementCompare);
	for(uint32 i = 0 ; i < content.size() ; i++)
		if(content[i]->IsDirectory()) ((FATDirectory*)content[i])->Sort();
}
