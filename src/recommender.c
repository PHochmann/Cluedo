/**
* @file recommender.c
*/


//***************************************************************************//
//************************** INCLUDES ***************************************//
//***************************************************************************//

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "recommender.h"
#include "sat.h"
#include "kb.h"

//***************************************************************************//
//************************** PRIVATE TYPEDEFS *******************************//
//***************************************************************************//

/**
 * @brief Todo
 * 
 * @var Guess::person Todo
 * @var Guess::weapon Todo
 * @var Guess::room Todo
 * @var Guess::h Todo
 */
typedef struct
{
    uint8_t person;
    uint8_t weapon;
    uint8_t room;
    float   h;
} Guess;

//***************************************************************************//
//************************** PRIVATE FUNCTION DECLARATIONS ******************//
//***************************************************************************//

/**
 * @brief Todo
 * 
 * @param env Todo
 * @param reachableRooms Todo
 * @param limit Todo
 * @return Todo
 */
static Guess* guessHeuristic(bool env[NUM_PERSONS][NUM_WEAPONS][NUM_ROOMS], const bool reachableRooms[NUM_ROOMS], size_t limit);

//***************************************************************************//
//************************** PRIVATE FUNCTION DEFINITIONS *******************//
//***************************************************************************//

static Guess* guessHeuristic(bool env[NUM_PERSONS][NUM_WEAPONS][NUM_ROOMS], const bool reachableRooms[NUM_ROOMS], size_t limit)
{
    Guess* candidates = calloc(limit, sizeof(Guess));
    uint32_t pP[NUM_PERSONS] = { 0 };
    uint32_t pW[NUM_WEAPONS] = { 0 };
    uint32_t pR[NUM_ROOMS]   = { 0 };

    for(uint8_t ep = 0u; ep < NUM_PERSONS; ep++)
    {
        for(uint8_t ew = 0u; ew < NUM_WEAPONS; ew++)
        {
            for(uint8_t er = 0u; er < NUM_ROOMS; er++)
            {
                if(reachableRooms[er] == false)
                {
                    continue;
                }
                if(env[ep][ew][er] == true)
                {
                    pP[ep]++;
                    pW[ew]++;
                    pR[er]++;
                }
            }
        }
    }
    for(uint8_t p = 0u; p < NUM_PERSONS; p++)
    {
        for(uint8_t w = 0u; w < NUM_WEAPONS; w++)
        {
            for(uint8_t r = 0u; r < NUM_ROOMS; r++)
            {
                if(reachableRooms[r] == false)
                {
                    continue;
                }

                float h = pP[p] + pW[w] + pR[r];
                uint32_t minIdx = 0u;
                for(uint32_t i = 1u; i < limit; i++)
                {
                    if(candidates[i].h < candidates[minIdx].h)
                    {
                        minIdx = i;
                    }
                }
                if(h > candidates[minIdx].h)
                {
                    candidates[minIdx].h      = h;
                    candidates[minIdx].person = p;
                    candidates[minIdx].weapon = w;
                    candidates[minIdx].room   = r;
                }
            }
        }
    }

    return candidates;
}

//***************************************************************************//
//************************** PUBLIC FUNCTION DEFINITIONS *******************//
//***************************************************************************//

void recommender_getGuess(GameState* game, uint8_t* outP, uint8_t* outW, uint8_t* outR, const bool* reachableRooms)
{
    SAT_Problem* kb           = game->kb;
    float        minEnvelopes = NUM_P_W_R_COMBINATONS + 1;
    uint8_t      minP         = 0;
    uint8_t      minW         = 0;
    uint8_t      minR         = 0;

    bool env[NUM_PERSONS][NUM_WEAPONS][NUM_ROOMS] = { false };
    for(uint8_t ep = 0u; ep < NUM_PERSONS; ep++)
    {
        if(game->currAssignment[kb_getVar(kb_getPersonCard(ep), ENVELOPE_PLAYER_ID)] == -1)
        {
            continue;
        }
        for(uint8_t ew = 0u; ew < NUM_WEAPONS; ew++)
        {
            if(game->currAssignment[kb_getVar(kb_getWeaponCard(ew), ENVELOPE_PLAYER_ID)] == -1)
            {
                continue;
            }
            for(uint8_t er = 0u; er < NUM_ROOMS; er++)
            {
                if(game->currAssignment[kb_getVar(kb_getRoomCard(er), ENVELOPE_PLAYER_ID)] == -1)
                {
                    continue;
                }
                sat_pushLevel(kb);
                sat_addUnitClause(kb, kb_getLiteral(kb_getPersonCard(ep), ENVELOPE_PLAYER_ID));
                sat_addUnitClause(kb, kb_getLiteral(kb_getWeaponCard(ew), ENVELOPE_PLAYER_ID));
                sat_addUnitClause(kb, kb_getLiteral(kb_getRoomCard(er), ENVELOPE_PLAYER_ID));
                if(sat_isSatisfiable(kb, NULL, NULL, NULL))
                {
                    env[ep][ew][er] = true;
                }
                sat_popLevel(kb);
            }
        }
    }

    uint32_t limit   = 5u;
    Guess*   guesses = guessHeuristic(env, reachableRooms, limit);

    for(uint32_t i = 0u; i < limit; i++)
    {
        uint8_t p = guesses[i].person;
        uint8_t w = guesses[i].weapon;
        uint8_t r = guesses[i].room;

        if(reachableRooms[r] == false)
        {
            continue;
        }

        uint32_t numResponses = 0u;
        uint32_t numEnvelopes = 0u;
        for(uint8_t pat = 0u; pat < (1u << (game->numPlayers - 1u)); pat++)
        {
            if(((pat >> (game->player - 1u)) & 1) == 1u)
            {
                continue;
            }
            sat_pushLevel(kb);
            for(uint8_t pl = 1u; pl < game->numPlayers; pl++)
            {
                if(game->player == pl)
                {
                    continue;
                }
                kb_addGuessAnswerClauses(game, p, w, r, pl, ((pat >> (pl - 1u)) & 1));
            }
            if(sat_isSatisfiable(kb, NULL, NULL, NULL) == false)
            {
                sat_popLevel(kb);
                continue;
            }
            numResponses++;
            for(uint8_t ep = 0u; ep < NUM_PERSONS; ep++)
            {
                for(uint8_t ew = 0u; ew < NUM_WEAPONS; ew++)
                {
                    for(uint8_t er = 0u; er < NUM_ROOMS; er++)
                    {
                        if(env[ep][ew][er] == true)
                        {
                            sat_pushLevel(kb);
                            sat_addUnitClause(kb, kb_getLiteral(kb_getPersonCard(ep), ENVELOPE_PLAYER_ID));
                            sat_addUnitClause(kb, kb_getLiteral(kb_getWeaponCard(ew), ENVELOPE_PLAYER_ID));
                            sat_addUnitClause(kb, kb_getLiteral(kb_getRoomCard(er), ENVELOPE_PLAYER_ID));
                            if(sat_isSatisfiable(kb, NULL, NULL, NULL) == true)
                            {
                                numEnvelopes++;
                            }
                            sat_popLevel(kb);
                        }
                    }
                }
            }
            sat_popLevel(kb);
        }

        float avgEnvelopes = (float)numEnvelopes / numResponses;
        if(avgEnvelopes < minEnvelopes)
        {
            minP         = p;
            minW         = w;
            minR         = r;
            minEnvelopes = avgEnvelopes;
        }
#if defined(DEBUG)
        printf("%d %d %d\n", p, w, r);
        printf("%f = %u / %u (min: %f)\n", avgEnvelopes, numEnvelopes, numResponses, minEnvelopes);
#endif
    }

    free(guesses);

    *outP = minP;
    *outW = minW;
    *outR = minR;
}

bool recommender_getKeyGuess(const GameState* game, uint8_t* outP, uint8_t* outC)
{
    for(uint8_t i = 1u; i < game->numPlayers; i++)
    {
        for(uint8_t j = 0u; j < NUM_CARDS; j++)
        {
            if(game->currAssignment[kb_getVar(j, ENVELOPE_PLAYER_ID)] == 0)
            {
                if(game->currAssignment[kb_getVar(j, i)] == 0)
                {
                    *outP = i;
                    *outC = j;
                    return true;
                }
            }
        }
    }
    return false; // Should only happen if envelope is already decided
}

bool recommender_isEnvelopeDecided(const GameState* game, uint8_t* outP, uint8_t* outW, uint8_t* outR)
{
    uint8_t found = 0u;
    for(uint8_t i = 0u; i < NUM_PERSONS; i++)
    {
        if(game->currAssignment[kb_getVar(kb_getPersonCard(i), ENVELOPE_PLAYER_ID)] == 1)
        {
            *outP = i;
            found++;
            break;
        }
    }
    for(uint8_t i = 0u; i < NUM_WEAPONS; i++)
    {
        if(game->currAssignment[kb_getVar(kb_getWeaponCard(i), ENVELOPE_PLAYER_ID)] == 1)
        {
            *outW = i;
            found++;
            break;
        }
    }
    for(uint8_t i = 0u; i < NUM_ROOMS; i++)
    {
        if(game->currAssignment[kb_getVar(kb_getRoomCard(i), ENVELOPE_PLAYER_ID)] == 1)
        {
            *outR = i;
            found++;
            break;
        }
    }
    return (found == 3u);
}
