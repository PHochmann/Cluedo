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
 * @brief Recommends the optimal guess to minimize the expected number of remaining envelope candidates.
 * 
 * @param game Current game state.
 * @param outP Output: recommended person card index.
 * @param outW Output: recommended weapon card index.
 * @param outR Output: recommended room card index.
 * @param reachableRooms Boolean array (size NUM_ROOMS) indicating which rooms are reachable this turn.
 */
void recommender_getGuess(GameState* game, uint8_t* outP, uint8_t* outW, uint8_t* outR, const bool* reachableRooms);

/**
 * @brief Finds a player/card pair for a "key guess": asking a specific player about a card that is
 *        still ambiguous (not yet confirmed absent from both that player and the envelope).
 * 
 * @param game Current game state.
 * @param outP Output: index of the player to ask.
 * @param outC Output: index of the card to ask about.
 * @return true if a useful key guess was found, false if no ambiguity remains.
 */
bool recommender_getKeyGuess(const GameState* game, uint8_t* outP, uint8_t* outC);

/**
 * @brief Checks whether the envelope (solution) has been fully determined by the knowledge base.
 * 
 * @param game Current game state.
 * @param outP Output: solution person card index if decided.
 * @param outW Output: solution weapon card index if decided.
 * @param outR Output: solution room card index if decided.
 * @return true if all three envelope cards are uniquely determined, false otherwise.
 */
bool recommender_isEnvelopeDecided(const GameState* game, uint8_t* outP, uint8_t* outW, uint8_t* outR);
