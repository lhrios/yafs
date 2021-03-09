
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

#include "command_line_parser.h"

#include <cctype>
#include <iostream>

void CommandLineParser::parseRules(const char* rules) {
	/* Format:
		i::? - optional with optional argument
		b: - required with required argument
		o? - no argument and optional
	*/

	int colon_count = 0;
	CommandLineOption option;
	char option_char = '\0';
	char c;

	valid = true;

	for(int i = 0; (c = rules[i]) != '\0' && valid; i++) {
		if (c == '?') {
			if (option_char == '\0') {
				valid = false;
			} else {
				option.required = false;
			}

		} else if (std::isalpha(c)) {
			if (option_char != '\0') {
				addToOptions(colon_count, option_char, option);
			}

			if (valid) {
				option_char = c;
				colon_count = 0;
				option = CommandLineOption();
			}

		} else if (c == ':') {
			if (++colon_count > 2) {
				valid = false;
			}

		} else {
			valid = false;
		}
	}

	if (option_char != '\0' && valid) {
		addToOptions(colon_count, option_char, option);
	}
}

void CommandLineParser::addToOptions(int colon_count, char option_char, CommandLineOption &option) {
	option.has_argument = colon_count > 0;
	option.required_argument = colon_count == 1;

	if (options.find(option_char) == options.end()) {
		options[option_char] = option;

	} else {
		valid = false;
	}
}

void CommandLineParser::parse(int argc, char** argv, const char* rules) {
	parseRules(rules);

	char last_option_char = '\0';

	for (int i = 1; i < argc && valid; i++) {
		char *option_str = argv[i];
		size_t option_length = strlen(option_str);

		if (option_length == 2 && option_str[0] == '-' && std::isalpha(option_str[1])
			&& options.find(option_str[1]) != options.end()) {

			if (last_option_char != '\0') {
				CommandLineOption &option = options.find(last_option_char)->second;
				if (option.required_argument) {
					valid = false;
					break;
				}
			}

			last_option_char = option_str[1];
			CommandLineOption &option = options.find(last_option_char)->second;
			option.found = true;

		} else {
			if (last_option_char != '\0') {
				CommandLineOption &option = options.find(last_option_char)->second;
				if (option.has_argument) {
					option.argument_value = option_str;
					last_option_char = '\0';

				} else {
					valid = false;
				}

			} else {
				valid = false;
			}
		}
	}

	if (last_option_char != '\0') {
		CommandLineOption &option = options.find(last_option_char)->second;
		if (option.required_argument) {
			valid = false;
		}
	}

	for (std::map<char, CommandLineOption>::const_iterator i = options.begin(); i != options.end() && valid; ++i) {
		const CommandLineOption &option = i->second;
		if ((option.required && !option.found) || (option.required_argument && !option.has_argument)) {
			valid = false;
		}
	}
}
