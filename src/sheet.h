/**
 * @file sheet.h
 */


#pragma once

//***************************************************************************//
//************************** INCLUDES ***************************************//
//***************************************************************************//

#include "kb.h"

//***************************************************************************//
//************************** PUBLIC FUNCTION DECLARATIONS *******************//
//***************************************************************************//

/**
 * @brief Prints the current deduction sheet.
 *
 * @param game Current game state.
 * @param settings Immutable game settings.
 */
void sheet_print(const GameState* game, const GameSettings* settings);
