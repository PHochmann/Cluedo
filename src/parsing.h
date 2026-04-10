/**
 * @file parsing.h
 */


#pragma once

//***************************************************************************//
//************************** INCLUDES ***************************************//
//***************************************************************************//

#include <stdint.h>
#include <stdarg.h>

#include "kb.h"


//***************************************************************************//
//**************************         PUBLIC DEFINES        ******************//
//***************************************************************************//

#define PARSING_NO_KEY_PLAYER -2

//***************************************************************************//
//************************** PUBLIC FUNCTION DECLARATIONS *******************//
//***************************************************************************//

/**
 * @brief Case-insensitive string comparison.
 *
 * @param s1 First string.
 * @param s2 Second string.
 * @return Negative if s1 < s2, 0 if equal, positive if s1 > s2.
 */
int parsing_strcmp2(const char* s1, const char* s2);

/**
 * @brief Reads a line from user input using a formatted prompt.
 *
 * @param promptfmt Printf-style prompt format string.
 * @return Dynamically allocated string (caller must free).
 */
char* parsing_readLine(const char* promptfmt, ...);

/**
 * @brief Gets a suspect selection from user input with validation.
 *
 * @param settings Immutable game settings.
 * @param promptfmt Printf-style prompt format string.
 * @return Suspect index.
 */
uint8_t parsing_getSuspect(const GameSettings* settings, const char* promptfmt, ...);

/**
 * @brief Gets a player selection from user input with validation.
 *
 * @param settings Immutable game settings.
 * @param promptfmt Printf-style prompt format string.
 * @return Player index.
 */
uint8_t parsing_getPlayer(const GameSettings* settings, const char* promptfmt, ...);

/**
 * @brief Gets a player or "none" selection from user input with validation.
 *
 * @param settings Immutable game settings.
 * @param promptfmt Printf-style prompt format string.
 * @return Player index or PARSING_NO_KEY_PLAYER for "none".
 */
int8_t parsing_getKeyPlayer(const GameSettings* settings, const char* promptfmt, ...);

/**
 * @brief Gets a weapon selection from user input with validation.
 *
 * @param settings Immutable game settings.
 * @param promptfmt Printf-style prompt format string.
 * @return Weapon index.
 */
uint8_t parsing_getWeapon(const GameSettings* settings, const char* promptfmt, ...);

/**
 * @brief Gets a room selection from user input with validation.
 *
 * @param settings Immutable game settings.
 * @param promptfmt Printf-style prompt format string.
 * @return Room index.
 */
uint8_t parsing_getRoom(const GameSettings* settings, const char* promptfmt, ...);

/**
 * @brief Gets a card selection from user input with validation.
 *
 * @param settings Immutable game settings.
 * @param promptfmt Printf-style prompt format string.
 * @return Card index.
 */
uint8_t parsing_getCard(const GameSettings* settings, const char* promptfmt, ...);

/**
 * @brief Gets a yes/no response from user input with validation.
 *
 * @param promptfmt Printf-style prompt format string.
 * @return true for yes, false for no.
 */
bool parsing_getYesNo(const char* promptfmt, ...);

/**
 * @brief Gets a dice roll from user input with validation.
 *
 * @param promptfmt Printf-style prompt format string.
 * @return Dice value in [1, 3].
 */
uint8_t parsing_getDiceRoll(const char* promptfmt, ...);

/**
 * @brief Gets a card count from user input with validation.
 *
 * @param settings Immutable game settings.
 * @param promptfmt Printf-style prompt format string.
 * @return Card count in [3, 5].
 */
uint8_t parsing_getCardNumber(const GameSettings* settings, const char* promptfmt, ...);
