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
 * Command Line Parser Module: defines a very simple parser to extract command line options and arguments.
 */

#ifndef COMMAND_LINE_PARSER
	#define COMMAND_LINE_PARSER

	#include <map>
	#include <cstring>

	#include <iostream>

	class CommandLineParser {
		public:
			class CommandLineOption {
				public:
					CommandLineOption() {
						memset(this, 0, sizeof(CommandLineOption));
						required = true;
					}

					~CommandLineOption() {
					}

					char *argument_value;
					bool found;

				private:
					bool has_argument;
					bool required_argument;
					bool required;

					friend class CommandLineParser;
			};

			CommandLineParser(int argc, char** argv, const char* rules) {
				parse(argc, argv, rules);
			}

			~CommandLineParser() {
			}

			inline bool isValid() {
				return valid;
			}

			const CommandLineOption* getOption(char c) {
				if (options.find(c) != options.end()) {
					return &options.find(c)->second;
				}
				return NULL;
			}

		private:
			bool valid;
			std::map<char, CommandLineOption> options;

			void parse(int argc, char** argv, const char* rules);
			void addToOptions(int colon_count, char option_char, CommandLineOption &option);
			void parseRules(const char* rules);
	};

#endif
