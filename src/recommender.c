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

#if defined(DEBUG)
#include "cards.h"
#endif

//***************************************************************************//
//************************** PRIVATE TYPEDEFS *******************************//
//***************************************************************************//

/** @brief Number of heuristic guesses to consider. A lower number reduces computation time. */
#define NUM_HEURISTIC_GUESSES 5u

/** @brief Total number of person/weapon/room combinations. */
#define NUM_P_W_R_COMBINATONS (NUM_PERSONS * NUM_WEAPONS * NUM_ROOMS)

/**
 * @brief Represents a candidate guess (person/weapon/room combination) with a heuristic score.
 * 
 * @var Guess::person Person card index.
 * @var Guess::weapon Weapon card index.
 * @var Guess::room Room card index.
 * @var Guess::h Heuristic score (higher is more informative).
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
 * @brief Builds the env array indicating which person/weapon/room combinations are still possible
 *        as the envelope solution based on the current knowledge base assignment.
 * 
 * @param game Current game state.
 * @param env Output: 3D boolean array [NUM_PERSONS][NUM_WEAPONS][NUM_ROOMS] to be populated.
 */
static void buildEnvelopeArray(const GameState* game, bool env[NUM_PERSONS][NUM_WEAPONS][NUM_ROOMS]);

/**
 * @brief Selects the top `limit` candidate guesses based on a marginal-frequency heuristic over
 *        still-possible envelope assignments.
 * 
 * @param env 3D boolean array [NUM_PERSONS][NUM_WEAPONS][NUM_ROOMS] indicating which combinations
 *            are still possible as the envelope solution.
 * @param reachableRooms Boolean array (size NUM_ROOMS) indicating which rooms are reachable this turn.
 * @param limit Maximum number of candidate guesses to return.
 * @return Heap-allocated array of the `limit` best candidate guesses (caller must free).
 */
static Guess* guessHeuristic(bool env[NUM_PERSONS][NUM_WEAPONS][NUM_ROOMS], const bool reachableRooms[NUM_ROOMS], size_t limit);

/**
 * @brief Adds clauses to the knowledge base to represent the assumption that the given person/weapon/room
 *        combination is the actual solution in the envelope.
 * 
 * @param kb Knowledge base to add clauses to.
 * @param p Person card index.
 * @param w Weapon card index.
 * @param r Room card index.
 */
static void addEnvelopeClauses(SAT_Problem* kb, uint8_t p, uint8_t w, uint8_t r);

//***************************************************************************//
//************************** PRIVATE FUNCTION DEFINITIONS *******************//
//***************************************************************************//

static void buildEnvelopeArray(const GameState* game, bool env[NUM_PERSONS][NUM_WEAPONS][NUM_ROOMS])
{
    SAT_Problem* kb = game->kb;
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
                addEnvelopeClauses(kb, ep, ew, er);
                if(sat_isSatisfiable(kb, NULL, NULL, NULL) == true)
                {
                    env[ep][ew][er] = true;
                }
                sat_popLevel(kb);
            }
        }
    }
}

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
                    candidates[minIdx] = (Guess){
                        .person = p,
                        .weapon = w,
                        .room   = r,
                        .h      = h
                    };
                }
            }
        }
    }

    return candidates;
}

void addEnvelopeClauses(SAT_Problem* kb, uint8_t p, uint8_t w, uint8_t r)
{
    sat_addUnitClause(kb, kb_getLiteral(kb_getPersonCard(p), ENVELOPE_PLAYER_ID));
    sat_addUnitClause(kb, kb_getLiteral(kb_getWeaponCard(w), ENVELOPE_PLAYER_ID));
    sat_addUnitClause(kb, kb_getLiteral(kb_getRoomCard(r), ENVELOPE_PLAYER_ID));
}

//***************************************************************************//
//************************** PUBLIC FUNCTION DEFINITIONS *******************//
//***************************************************************************//

void recommender_getGuess(GameState* game, uint8_t* outP, uint8_t* outW, uint8_t* outR, const bool* reachableRooms)
{
    SAT_Problem* kb           = game->kb;
    uint32_t     minEnvelopes = NUM_P_W_R_COMBINATONS + 1;
    uint8_t      minP         = 0;
    uint8_t      minW         = 0;
    uint8_t      minR         = 0;

    bool env[NUM_PERSONS][NUM_WEAPONS][NUM_ROOMS] = { false };
    buildEnvelopeArray(game, env);

    Guess* guesses = guessHeuristic(env, reachableRooms, NUM_HEURISTIC_GUESSES);

    for(uint32_t i = 0u; i < NUM_HEURISTIC_GUESSES; i++)
    {
        uint8_t p = guesses[i].person;
        uint8_t w = guesses[i].weapon;
        uint8_t r = guesses[i].room;

        uint32_t maxEnvelopes = 0u;
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
            uint32_t numEnvelopes = 0u;
            for(uint8_t ep = 0u; ep < NUM_PERSONS; ep++)
            {
                for(uint8_t ew = 0u; ew < NUM_WEAPONS; ew++)
                {
                    for(uint8_t er = 0u; er < NUM_ROOMS; er++)
                    {
                        if(env[ep][ew][er] == true)
                        {
                            sat_pushLevel(kb);
                            addEnvelopeClauses(kb, ep, ew, er);
                            if(sat_isSatisfiable(kb, NULL, NULL, NULL) == true)
                            {
                                numEnvelopes++;
                            }
                            sat_popLevel(kb);
                        }
                    }
                }
            }
            if(numEnvelopes > maxEnvelopes)
            {
                maxEnvelopes = numEnvelopes;
            }
            sat_popLevel(kb);
        }

        if(maxEnvelopes < minEnvelopes)
        {
            minP         = p;
            minW         = w;
            minR         = r;
            minEnvelopes = maxEnvelopes;
        }
#if defined(DEBUG)
        printf("%s %s %s\n", cards_getPersonName(p), cards_getWeaponName(w), cards_getRoomName(r));
        printf("%u (min: %u)\n", maxEnvelopes, minEnvelopes);
#endif
    }

    free(guesses);

    *outP = minP;
    *outW = minW;
    *outR = minR;
}

bool recommender_getKeyGuess(const GameState* game, uint8_t* outP, uint8_t* outC)
{
    SAT_Problem* kb = game->kb;

    bool env[NUM_PERSONS][NUM_WEAPONS][NUM_ROOMS] = { false };
    buildEnvelopeArray(game, env);

    // Try each card and player to find the best key guess
    float minAvgEnvelopes = NUM_P_W_R_COMBINATONS + 1;
    uint8_t  bestPlayer      = 0;
    uint8_t  bestCard        = 0;

    for(uint8_t c = 0u; c < NUM_CARDS; c++)
    {
        for(uint8_t pl = 1u; pl < game->numPlayers; pl++)
        {
            if(game->currAssignment[kb_getVar(c, pl)] != 0)
            {
                // Only consider cards that aren't already known
                continue;
            }

            // Count envelopes if player has this card
            uint32_t numEnvelopesYes = 0u;
            sat_pushLevel(kb);
            sat_addUnitClause(kb, kb_getLiteral(c, pl));
            for(uint8_t ep = 0u; ep < NUM_PERSONS; ep++)
            {
                for(uint8_t ew = 0u; ew < NUM_WEAPONS; ew++)
                {
                    for(uint8_t er = 0u; er < NUM_ROOMS; er++)
                    {
                        if(env[ep][ew][er] == true)
                        {
                            sat_pushLevel(kb);
                            addEnvelopeClauses(kb, ep, ew, er);
                            if(sat_isSatisfiable(kb, NULL, NULL, NULL) == true)
                            {
                                numEnvelopesYes++;
                            }
                            sat_popLevel(kb);
                        }
                    }
                }
            }
            sat_popLevel(kb);

            // Count envelopes if player doesn't have this card
            uint32_t numEnvelopesNo = 0u;
            sat_pushLevel(kb);
            sat_addUnitClause(kb, -kb_getLiteral(c, pl));
            for(uint8_t ep = 0u; ep < NUM_PERSONS; ep++)
            {
                for(uint8_t ew = 0u; ew < NUM_WEAPONS; ew++)
                {
                    for(uint8_t er = 0u; er < NUM_ROOMS; er++)
                    {
                        if(env[ep][ew][er] == true)
                        {
                            sat_pushLevel(kb);
                            addEnvelopeClauses(kb, ep, ew, er);
                            if(sat_isSatisfiable(kb, NULL, NULL, NULL) == true)
                            {
                                numEnvelopesNo++;
                            }
                            sat_popLevel(kb);
                        }
                    }
                }
            }
            sat_popLevel(kb);
#if defined(DEBUG)
            printf("Card %s, Player %s: Yes=%u, No=%u (curr. avg=%f)\n", cards_getCardName(c), game->playerNames[pl], numEnvelopesYes, numEnvelopesNo, minAvgEnvelopes);
#endif
            float avgEnvelopes = (numEnvelopesYes + numEnvelopesNo) / 2.0f;
            
            if(avgEnvelopes < minAvgEnvelopes)
            {
                minAvgEnvelopes = avgEnvelopes;
                bestPlayer      = pl;
                bestCard        = c;
            }
        }
    }

    *outP = bestPlayer;
    *outC = bestCard;
    return true;
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
