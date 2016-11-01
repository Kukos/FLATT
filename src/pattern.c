#include <pattern.h>
#include <log.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RED "\x1B[31m"
#define RESET "\x1B[0m"

/* ASCII Alphabet only printable chars */
#define FIRST_CHAR_CODE 8
#define LAST_CHAR_CODE 126
#define ALPHABET_SIZE (LAST_CHAR_CODE - FIRST_CHAR_CODE + 1)

#define FREE(PTR) do{ free(PTR); PTR = NULL; }while(0)

#define LETTER_TO_STATE(N) ( (N) > 0 ? (N) - FIRST_CHAR_CODE : 0)

/* Simple convert t[i][j] to t[INDEX(i,j.n)] to avoid 2D array*/
#define INDEX(I,J,N) ( (I) * (N) + (J) )

/*
    Function create FSM

    The main idea:

    Instead of calculating max prefix in each step,
    update the state with best prefix.

    Always if we have correct letter we go to the next step
    But if not, we have to come back to state with longest prefix of current sufix

    But how update state with longest prefix ?
    It simple but not trivial, we have to go to the current state with Longest prefix
    and go to the next state depends on cur char

    And we have to update state with longest prefix, because we change our state.

    So new_state_with_longest_prefix = old_state_with_longest_prefix[ current char ]

    To avoid calculating next state for all ALPHABET, we can copy states from
    state_with_longest_prefix, why ? beacuse it is the best state of our current state
    ( this works beacuse we update state_with_longest_prefix for each state ),
    after copy we update path to the next state ( depends on cur char )

    Of course to speed up we use 1D array to simulate 2D FSM array
    and SSE for operation on this array

    So we can create FSM states in O(ALPHABET_SIZE * PATTERN_LENGTH)

    PARAMS:
    IN pattern - pointer to pattern

    RETURN:
    %NULL if failure
    %pointer to FSM array if success
*/
static int *fsm_create(const char *pattern);

/*
    Create states using calculating longest prefix
    in pattern subsequences

    PARAMS
    @IN pattern - pointer to pattern

    RETURN:
    %NULL if failure
    %Pointer to states array if success
*/
static int *kmp_states_create(const char *pattern);

static int *fsm_create(const char *pattern)
{
    int *fsm; /* fsm state array */
    int len;
    int state;
    int state_with_max_prefix;

    __trace_call__(TRACE);

    if(pattern == NULL)
    {
        __log__(DEBUG);
        __log__("\tpattern = NULL\n");
        return NULL;
    }

    len = (int)strlen(pattern);
    if(! len)
    {
        __log__(DEBUG);
        __log__("\tlen == 0\n");
        return NULL;
    }

    /* Need state 0, so alloc len + 1 */
    fsm = (int*)malloc(sizeof(int) * ALPHABET_SIZE * (len + 1));
    if(fsm == NULL)
    {
        __log__(DEBUG);
        __log__("\tmalloc error\n");
        return NULL;
    }

    /* From init state we go to the next state IFF we match letter, so init first row */
    if(memset(fsm, 0, sizeof(int) * ALPHABET_SIZE) == NULL)
    {
        __log__(DEBUG);
        __log__("\tmemset error\n");

        FREE(fsm);
        return NULL;
    }

    fsm[INDEX(0, LETTER_TO_STATE(pattern[0]), ALPHABET_SIZE)] = 1;

    /* at the begining init state has longest prefix */
    state_with_max_prefix = 0;

    /* build states */
    for(state = 1; state <= len; ++state)
    {
        /* state != state_with_max_prefix so we can use memcpy instead of memmove */
        if( memcpy( fsm + INDEX(state, 0, ALPHABET_SIZE),
                fsm + INDEX(state_with_max_prefix, 0, ALPHABET_SIZE),
                sizeof(int) * ALPHABET_SIZE) == NULL )
            {
                __log__(DEBUG);
                __log__("\tmemcpy error\n");

                FREE(fsm);
                return NULL;
            }

        /* update path to the next state */
        fsm[INDEX(state, LETTER_TO_STATE(pattern[state]), ALPHABET_SIZE)] = state + 1;

        /* update state with longest prefix */
        state_with_max_prefix = fsm[INDEX(state_with_max_prefix,
                                    LETTER_TO_STATE(pattern[state]),
                                    ALPHABET_SIZE)];
    }

    return fsm;
}

static int *kmp_states_create(const char *pattern)
{
    int *states;
    int i;
    int len;
    int longest_prefix;

    __trace_call__(TRACE);

    if(pattern == NULL)
    {
        __log__(DEBUG);
        __log__("\tnpattern == NULL\n");
        return NULL;
    }

    len = (int)strlen(pattern);
    if(! len)
    {
        __log__(DEBUG);
        __log__("\tlen == 0\n");
        return NULL;
    }

    states = (int*)malloc(sizeof(int) * len);
    if(states == NULL)
    {
        __log__(DEBUG);
        __log__("\tmalloc error\n");
        return NULL;
    }

    /* start without prefix */
    longest_prefix = -1;

    /* 1 char has empty char as longest prefix */
    states[0] = longest_prefix;

    for(i = 1; i < len; ++i)
    {
		/* go back while pattern doesn't match */
      	while(longest_prefix > -1 && pattern[longest_prefix + 1] != pattern[i])
            longest_prefix = states[longest_prefix];

        /* here longest_prefix == 0 || pattern[i] == pattern[longest_prefix]
           if pattern matches we can extends prefix */
        if(pattern[i] == pattern[longest_prefix + 1])
            ++longest_prefix;

        /* we save state as longest_prefix */
        states[i] = longest_prefix;
    }

    return states;
}


void fsm(const char *text, const char *pattern)
{
    int *fsm_states; /* array of fsm states */
    int text_len;
    int pattern_len;
    int i;
    int state; /* current state */

    /* need to print results */
	int prev_end = 0; /* prev end of printed text */
    int end = 0; /* current end of text ( and start pattern ) */

    __trace_call__(TRACE);

    if(pattern == NULL || text == NULL)
    {
        __log__(DEBUG);
        __log__("\tpattern == NULL || text == NULL\n");
        return;
    }

    fsm_states = fsm_create(pattern);
    if(fsm_states == NULL)
    {
        __log__(DEBUG);
        __log__("\tfsm_create error\n");
        return;
    }

    text_len = (int)strlen(text);
    pattern_len = (int)strlen(pattern);

    /* start from init state */
    state = 0;

    end = 0;
    prev_end = 0;

    for(i = 0; i < text_len; ++i)
    {
        state = fsm_states[INDEX(state, LETTER_TO_STATE(text[i]), ALPHABET_SIZE)];
        if(state == pattern_len)
        {
            end = i - pattern_len + 1;

            /* to avoid print text when we have 2x pattern i.e alala and pat: ala */
            if(end > prev_end)
            {
                printf("%.*s", end - prev_end, text + prev_end);
                printf(RED "%s" RESET, pattern);

            }
            else
                printf(RED "%.*s" RESET, pattern_len - (prev_end - end), text + prev_end);

            prev_end = end + pattern_len;
        }
    }

    FREE(fsm_states);

    printf("%.*s\n", end != 0 ? end - prev_end : text_len , text + prev_end);
}

void kmp(const char *text, const char *pattern)
{
	int text_len;
	int pattern_len;
	int *states;
	int i;
	int matches; /* len of matched string */

    /* need to print results */
    int prev_end = 0; /* prev end of printed text */
    int end = 0; /* current end of text ( and start pattern ) */

	__trace_call__(TRACE);

    if(text == NULL || pattern == NULL)
    {
        __log__(DEBUG);
        __log__("\ttext == NULL || pattern == NULL\n");
        return;
    }

    states = kmp_states_create(pattern);

    if(states == NULL)
    {
        __log__(DEBUG);
        __log__("\tkmp_states_create error\n");
        return;
    }

    text_len = (int)strlen(text);
    pattern_len = (int)strlen(pattern);

    matches = -1;
    for(i = 0; i < text_len; ++i)
    {
        /* go back while pattern doesn't match text*/
        while(matches > -1 && pattern[matches + 1] != text[i])
			matches = states[matches];

        /* if matches then we can extends matched string*/
		if(pattern[matches + 1] == text[i])
			++matches;

		/* we find pattern in text */
		if(matches == pattern_len - 1)
        {

            end = i - pattern_len;
            /* to avoid print text when we have 2x pattern i.e alala and pat: ala */
            if(end > prev_end)
            {
                printf("%.*s", end - prev_end + 1,text + prev_end);
                printf(RED "%s" RESET, pattern);

            }
            else
                printf(RED "%.*s" RESET, pattern_len - (prev_end - end) + 1, text + prev_end);

            prev_end = end + pattern_len + 1;

        	matches = states[matches];
		}
    }

    FREE(states);

    printf("%.*s\n", end != 0 ? end - prev_end : text_len , text + prev_end);
}
