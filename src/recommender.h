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
//************************** PUBLIC FUNCTION DECLARATIONS *******************//
//***************************************************************************//

/**
 * @brief Recommends the optimal guess to minimize the expected number of remaining envelope candidates.
 * 
 * @param game Current game state.
 * @param settings Immutable game settings.
 * @param player Player making the guess.
 * @param outP Output: recommended suspect card index.
 * @param outW Output: recommended weapon card index.
 * @param outR Output: recommended room card index.
 * @param reachableRooms Boolean array (size NUM_ROOMS) indicating which rooms are reachable this turn.
 */
void recommender_getGuess(
    const GameSettings* settings,
    GameState*          game,
    const bool*         reachableRooms,
    uint8_t*            outP,
    uint8_t*            outW,
    uint8_t*            outR);

/**
 * @brief Finds a player/card pair for a "key guess": asking a specific player about a card that is
 *        still ambiguous (not yet confirmed absent from both that player and the envelope).
 * 
 * @param game Current game state.
 * @param settings Immutable game settings.
 * @param outP Output: player index to ask.
 * @param outC Output: card index to ask about.
 * @return true (the output pair is always set by the current implementation).
 */
bool recommender_getKeyGuess(GameState* game, const GameSettings* settings, uint8_t* outP, uint8_t* outC);

/**
 * @brief Checks whether the envelope (solution) has been fully determined by the knowledge base.
 * 
 * @param game Current game state.
 * @param outP Output: solution suspect card index if decided.
 * @param outW Output: solution weapon card index if decided.
 * @param outR Output: solution room card index if decided.
 * @return true if all three envelope cards are uniquely determined, false otherwise.
 */
bool recommender_isEnvelopeDecided(const GameState* game, uint8_t* outP, uint8_t* outW, uint8_t* outR);
