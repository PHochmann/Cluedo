/**
* @file recommender.h
*/


#pragma once

//***************************************************************************//
//************************** INCLUDES ***************************************//
//***************************************************************************//

#include <stdint.h>
#include <stdbool.h>

#include "kb.h"

//***************************************************************************//
//************************** PUBLIC TYPEDEFS ********************************//
//***************************************************************************//


//***************************************************************************//
//************************** PUBLIC VARIABLE DEFINITIONS ********************//
//***************************************************************************//


//***************************************************************************//
//************************** PUBLIC FUNCTION DECLARATIONS *******************//
//***************************************************************************//

/**
 * @brief Todo
 * 
 * @param game Todo
 * @param outP Todo
 * @param outW Todo
 * @param outR Todo
 * @param reachableRooms Todo
 */
void recommender_getGuess(GameState* game, uint8_t* outP, uint8_t* outW, uint8_t* outR, const bool* reachableRooms);

/**
 * @brief Todo
 * 
 * @param game Todo
 * @param outP Todo
 * @param outC Todo
 * @return Todo
 */
bool recommender_getKeyGuess(const GameState* game, uint8_t* outP, uint8_t* outC);

/**
 * @brief Todo
 * 
 * @param game Todo
 * @param outP Todo
 * @param outW Todo
 * @param outR Todo
 * @return Todo
 */
bool recommender_isEnvelopeDecided(const GameState* game, uint8_t* outP, uint8_t* outW, uint8_t* outR);
