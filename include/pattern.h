#ifndef PATTERN_H
#define PATTERN_H

/*
	Find pattern in text ( stdin  or file )

	Algorithms searching for pattern and print input text with matching pattern on stdout

	Author: Michal Kukowski
	email: michalkukowski10@gmail.com
	Licence: GPL3
*/

/*
	Knuth-Morris-Pratt Algorithm

	PARAMS
	@IN text - pointer to text
	@IN pattern - pointer to patter

	RETURN:
	This is a void function
*/
void kmp(const char *text, const char *pattern);

/*
	Finite State Machine

	PARAMS
	@IN text - pointer to text
	@IN pattern - pointer to patter

	RETURN:
	This is a void function

*/
void fsm(const char *text, const char *pattern);

#endif
