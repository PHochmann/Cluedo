/**
 * @file parsing.c
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>
#include <readline/readline.h>

#include "cards.h"
#include "kb.h"
#include "parsing.h"

//***************************************************************************//
//************************** PRIVATE FUNCTIONS ******************************//
//***************************************************************************//

/**
 * @brief Case-insensitive string comparison for internal parser helpers.
 *
 * @param s1 First string.
 * @param s2 Second string.
 * @return Negative if s1 < s2, 0 if equal, positive if s1 > s2.
 */
static int strcmp2_internal(const char* s1, const char* s2)
{
    while(*s1 && *s2)
    {
        int c1 = tolower((unsigned char)*s1);
        int c2 = tolower((unsigned char)*s2);

        if(c1 != c2)
        {
            return c1 - c2;
        }

        s1++;
        s2++;
    }

    // Handle different lengths
    return (unsigned char)tolower(*s1) -
           (unsigned char)tolower(*s2);
}

/**
 * @brief Reads a line with a formatted prompt using a va_list.
 *
 * @param fmt Printf-style prompt format string.
 * @param args Variadic argument list for fmt.
 * @return Dynamically allocated input string, or NULL if prompt allocation fails.
 */
static char* vreadlineasprintf_internal(const char* fmt, va_list args)
{
    char* prompt = NULL;
    char* line;
    if(vasprintf(&prompt, fmt, args) == -1)
    {
        return NULL;
    }
    line = readline(prompt);
    free(prompt);
    return line;
}

/**
 * @brief Runs the shared parse/validate input loop for parser frontends.
 *
 * @param parseFunc Validator callback used to parse and validate user input.
 * @param settings Immutable game settings forwarded to parseFunc.
 * @param fmt Printf-style prompt format string.
 * @param args Variadic argument list for fmt.
 * @return Parsed value from parseFunc.
 */
static int8_t vparse_internal(int8_t (*parseFunc)(const GameSettings* settings, const char* str), const GameSettings* settings, const char* fmt, va_list args)
{
    int8_t result = -1;
    while(result == -1)
    {
        va_list argsCpy;
        va_copy(argsCpy, args);
        char* input = vreadlineasprintf_internal(fmt, argsCpy);
        va_end(argsCpy);
        if(input != NULL)
        {
            result = parseFunc(settings, input);
            if(result == -1)
            {
                printf("Invalid input.\n");
            }
            free(input);
        }
    }
    return result;
}

// Individual validators (all static)

static int8_t parseSuspect_validator(const GameSettings* settings, const char* str)
{
    (void)settings;
    for(uint8_t i = 0u; i < NUM_SUSPECTS; i++)
    {
        if(strcmp2_internal(str, cards_getSuspectName(i)) == 0)
        {
            return i;
        }
    }
    return -1;
}

static int8_t parsePlayer_validator(const GameSettings* settings, const char* str)
{
    for(uint8_t i = 1u; i < settings->numPlayers; i++)
    {
        if(strcmp2_internal(str, settings->playerNames[i]) == 0)
        {
            return i;
        }
    }
    return -1;
}

static int8_t parseKeyPlayer_validator(const GameSettings* settings, const char* str)
{
    if(str[0] == 'n' || str[0] == 'N')
    {
        return PARSING_NO_KEY_PLAYER;
    }
    return parsePlayer_validator(settings, str);
}

static int8_t parseWeapon_validator(const GameSettings* settings, const char* str)
{
    (void)settings;
    for(uint8_t i = 0u; i < NUM_WEAPONS; i++)
    {
        if(strcmp2_internal(str, cards_getWeaponName(i)) == 0)
        {
            return i;
        }
    }
    return -1;
}

static int8_t parseRoom_validator(const GameSettings* settings, const char* str)
{
    (void)settings;
    for(uint8_t i = 0u; i < NUM_ROOMS; i++)
    {
        if(strcmp2_internal(str, cards_getRoomName(i)) == 0)
        {
            return i;
        }
    }
    return -1;
}

static int8_t parseCard_validator(const GameSettings* settings, const char* str)
{
    (void)settings;
    int8_t result;

    result = parseSuspect_validator(settings, str);
    if(result != -1)
    {
        return cards_getSuspectCard(result);
    }
    result = parseWeapon_validator(settings, str);
    if(result != -1)
    {
        return cards_getWeaponCard(result);
    }
    result = parseRoom_validator(settings, str);
    if(result != -1)
    {
        return cards_getRoomCard(result);
    }
    return -1;
}

static int8_t parseYesNo_validator(const GameSettings* settings, const char* str)
{
    (void)settings;
    if(str[0] == 'n' || str[0] == 'N')
    {
        return 0;
    }
    if(str[0] == 'y' || str[0] == 'Y')
    {
        return 1;
    }
    return -1;
}

static int8_t parseDiceRoll_validator(const GameSettings* settings, const char* str)
{
    (void)settings;
    int8_t value = atoi(str);
    if((value >= 1) && (value <= 3))
    {
        return value;
    }
    return -1;
}

static int8_t parseCardNumber_validator(const GameSettings* settings, const char* str)
{
    (void)settings;
    int8_t value = atoi(str);
    if((value >= 3) && (value <= 5))
    {
        return value;
    }
    return -1;
}

//***************************************************************************//
//************************** PUBLIC FUNCTION DEFINITIONS ********************//
//***************************************************************************//

int parsing_strcmp2(const char* s1, const char* s2)
{
    return strcmp2_internal(s1, s2);
}

char* parsing_readLine(const char* promptfmt, ...)
{
    va_list args;
    va_start(args, promptfmt);
    char* result = vreadlineasprintf_internal(promptfmt, args);
    va_end(args);
    return result;
}

uint8_t parsing_getSuspect(const GameSettings* settings, const char* promptfmt, ...)
{
    va_list args;
    va_start(args, promptfmt);
    uint8_t result = vparse_internal(parseSuspect_validator, settings, promptfmt, args);
    va_end(args);
    return result;
}

uint8_t parsing_getPlayer(const GameSettings* settings, const char* promptfmt, ...)
{
    va_list args;
    va_start(args, promptfmt);
    uint8_t result = vparse_internal(parsePlayer_validator, settings, promptfmt, args);
    va_end(args);
    return result;
}

int8_t parsing_getKeyPlayer(const GameSettings* settings, const char* promptfmt, ...)
{
    va_list args;
    va_start(args, promptfmt);
    int8_t result = vparse_internal(parseKeyPlayer_validator, settings, promptfmt, args);
    va_end(args);
    return result;
}

uint8_t parsing_getWeapon(const GameSettings* settings, const char* promptfmt, ...)
{
    va_list args;
    va_start(args, promptfmt);
    uint8_t result = vparse_internal(parseWeapon_validator, settings, promptfmt, args);
    va_end(args);
    return result;
}

uint8_t parsing_getRoom(const GameSettings* settings, const char* promptfmt, ...)
{
    va_list args;
    va_start(args, promptfmt);
    uint8_t result = vparse_internal(parseRoom_validator, settings, promptfmt, args);
    va_end(args);
    return result;
}

uint8_t parsing_getCard(const GameSettings* settings, const char* promptfmt, ...)
{
    va_list args;
    va_start(args, promptfmt);
    uint8_t result = vparse_internal(parseCard_validator, settings, promptfmt, args);
    va_end(args);
    return result;
}

bool parsing_getYesNo(const char* promptfmt, ...)
{
    va_list args;
    va_start(args, promptfmt);
    bool result = (vparse_internal(parseYesNo_validator, NULL, promptfmt, args) != 0);
    va_end(args);
    return result;
}

uint8_t parsing_getDiceRoll(const char* promptfmt, ...)
{
    va_list args;
    va_start(args, promptfmt);
    uint8_t result = vparse_internal(parseDiceRoll_validator, NULL, promptfmt, args);
    va_end(args);
    return result;
}

uint8_t parsing_getCardNumber(const GameSettings* settings, const char* promptfmt, ...)
{
    va_list args;
    va_start(args, promptfmt);
    uint8_t result = vparse_internal(parseCardNumber_validator, settings, promptfmt, args);
    va_end(args);
    return result;
}
