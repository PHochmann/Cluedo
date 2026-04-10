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
#include <math.h>

#include "recommender.h"
#include "cards.h"
#include "kb.h"

//***************************************************************************//
//**************************        PRIVATE DEFINES        ******************//
//***************************************************************************//

/** @brief Number of heuristic guesses to consider. A lower number reduces computation time. */
#define NUM_HEURISTIC_GUESSES 5u

/** @brief Total number of suspect/weapon/room combinations. */
#define NUM_P_W_R_COMBINATONS (NUM_SUSPECTS * NUM_WEAPONS * NUM_ROOMS)

//***************************************************************************//
//************************** PRIVATE TYPEDEFS *******************************//
//***************************************************************************//

/**
 * @brief Represents a candidate guess (suspect/weapon/room combination) with a heuristic score.
 * 
 * @var Guess::suspect Suspect card index.
 * @var Guess::weapon Weapon card index.
 * @var Guess::room Room card index.
 * @var Guess::h Heuristic score (higher is more informative).
 */
typedef struct
{
    uint8_t suspect;
    uint8_t weapon;
    uint8_t room;
    float   h;
} Guess;

//***************************************************************************//
//************************** PRIVATE FUNCTION DECLARATIONS ******************//
//***************************************************************************//

/**
 * @brief Builds the env array indicating which suspect/weapon/room combinations are still possible
 *        as the envelope solution based on the current knowledge base assignment.
 * 
 * @param game Current game state.
 * @param env Output: 3D boolean array [NUM_SUSPECTS][NUM_WEAPONS][NUM_ROOMS] to be populated.
 */
static void buildEnvelopeArray(const GameState* game, bool env[NUM_SUSPECTS][NUM_WEAPONS][NUM_ROOMS]);

/**
 * @brief Selects the top `limit` candidate guesses based on a marginal-frequency heuristic over
 *        still-possible envelope assignments.
 * 
 * @param env 3D boolean array [NUM_SUSPECTS][NUM_WEAPONS][NUM_ROOMS] indicating which combinations
 *            are still possible as the envelope solution.
 * @param reachableRooms Boolean array (size NUM_ROOMS) indicating which rooms are reachable this turn.
 * @param limit Maximum number of candidate guesses to return.
 * @return Heap-allocated array of the `limit` best candidate guesses (caller must free).
 */
static Guess* guessHeuristic(const bool env[NUM_SUSPECTS][NUM_WEAPONS][NUM_ROOMS], const bool reachableRooms[NUM_ROOMS], size_t limit);

/**
 * @brief Counts how many currently-possible envelope combinations remain satisfiable
 *        under the current temporary KB assumptions.
 *
 * @param game Current game state.
 * @param env 3D boolean array [NUM_SUSPECTS][NUM_WEAPONS][NUM_ROOMS] indicating envelope candidates.
 * @return Number of satisfiable envelope candidates.
 */
static uint32_t countPossibleEnvelopes(const GameState* game, const bool env[NUM_SUSPECTS][NUM_WEAPONS][NUM_ROOMS]);

/**
 * @brief Checks satisfiability under a temporary envelope assumption.
 *
 * @param game Current game state.
 * @param p Suspect card index.
 * @param w Weapon card index.
 * @param r Room card index.
 * @return true if satisfiable under the assumption, false otherwise.
 */
static bool isSatisfiableWithEnvelopeAssumption(const GameState* game, uint8_t p, uint8_t w, uint8_t r);

/**
 * @brief Counts remaining envelope candidates for one answer pattern of a candidate guess.
 *
 * @param game Current game state.
 * @param settings Immutable game settings.
 * @param player Guessing player index.
 * @param env Envelope candidate mask.
 * @param p Candidate suspect card index.
 * @param w Candidate weapon card index.
 * @param r Candidate room card index.
 * @param answerPattern Bit pattern of yes/no answers for players (excluding envelope).
 * @return Number of satisfiable envelope candidates; 0 if the pattern is infeasible.
 */
static uint32_t countPossibleEnvelopesForGuessPattern(
    GameState*          game,
    const GameSettings* settings,
    uint8_t             player,
    const bool          env[NUM_SUSPECTS][NUM_WEAPONS][NUM_ROOMS],
    uint8_t             p,
    uint8_t             w,
    uint8_t             r,
    uint8_t             pattern);

//***************************************************************************//
//************************** PRIVATE FUNCTION DEFINITIONS *******************//
//***************************************************************************//

static bool isSatisfiableWithEnvelopeAssumption(const GameState* game, uint8_t p, uint8_t w, uint8_t r)
{
    return kb_isSatisfiableWithAssumptions(game, 3u, (SAT_Literal[3]){ kb_getLiteral(cards_getSuspectCard(p), ENVELOPE_PLAYER_ID), kb_getLiteral(cards_getWeaponCard(w), ENVELOPE_PLAYER_ID), kb_getLiteral(cards_getRoomCard(r), ENVELOPE_PLAYER_ID) });
}

static void buildEnvelopeArray(const GameState* game, bool env[NUM_SUSPECTS][NUM_WEAPONS][NUM_ROOMS])
{
    for(uint8_t ep = 0u; ep < NUM_SUSPECTS; ep++)
    {
        if(kb_getAssignmentValue(game, cards_getSuspectCard(ep), ENVELOPE_PLAYER_ID) == SAT_FALSE)
        {
            continue;
        }
        for(uint8_t ew = 0u; ew < NUM_WEAPONS; ew++)
        {
            if(kb_getAssignmentValue(game, cards_getWeaponCard(ew), ENVELOPE_PLAYER_ID) == SAT_FALSE)
            {
                continue;
            }
            for(uint8_t er = 0u; er < NUM_ROOMS; er++)
            {
                if(kb_getAssignmentValue(game, cards_getRoomCard(er), ENVELOPE_PLAYER_ID) == SAT_FALSE)
                {
                    continue;
                }
                if(isSatisfiableWithEnvelopeAssumption((GameState*)game, ep, ew, er) == true)
                {
                    env[ep][ew][er] = true;
                }
            }
        }
    }
}

static uint32_t countPossibleEnvelopes(const GameState* game, const bool env[NUM_SUSPECTS][NUM_WEAPONS][NUM_ROOMS])
{
    uint32_t numEnvelopes = 0u;
    for(uint8_t ep = 0u; ep < NUM_SUSPECTS; ep++)
    {
        for(uint8_t ew = 0u; ew < NUM_WEAPONS; ew++)
        {
            for(uint8_t er = 0u; er < NUM_ROOMS; er++)
            {
                if(env[ep][ew][er] == false)
                {
                    continue;
                }
                if(isSatisfiableWithEnvelopeAssumption(game, ep, ew, er) == true)
                {
                    numEnvelopes++;
                }
            }
        }
    }

    return numEnvelopes;
}

static uint32_t countPossibleEnvelopesForGuessPattern(
    GameState*          game,
    const GameSettings* settings,
    uint8_t             player,
    const bool          env[NUM_SUSPECTS][NUM_WEAPONS][NUM_ROOMS],
    uint8_t             p,
    uint8_t             w,
    uint8_t             r,
    uint8_t             answerPattern)
{
    kb_pushLevel(game);

    for(uint8_t pl = 1u; pl < settings->numPlayers; pl++)
    {
        if(player == pl)
        {
            continue;
        }
        kb_addGuessAnswerClauses(game, p, w, r, pl, ((answerPattern >> (pl - 1u)) & 1u), false);
    }

    if(kb_isSatisfiable(game) == false)
    {
        kb_popLevel(game);
        return 0u;
    }

    uint32_t numEnvelopes = countPossibleEnvelopes(game, env);
    kb_popLevel(game);
    return numEnvelopes;
}

static Guess* guessHeuristic(const bool env[NUM_SUSPECTS][NUM_WEAPONS][NUM_ROOMS], const bool reachableRooms[NUM_ROOMS], size_t limit)
{
    Guess* candidates = calloc(limit, sizeof(Guess));

    // Frequency counts of cards in still-possible envelopes for heuristic scoring
    uint32_t pP[NUM_SUSPECTS] = { 0 };
    uint32_t pW[NUM_WEAPONS]  = { 0 };
    uint32_t pR[NUM_ROOMS]    = { 0 };

    for(uint8_t ep = 0u; ep < NUM_SUSPECTS; ep++)
    {
        for(uint8_t ew = 0u; ew < NUM_WEAPONS; ew++)
        {
            for(uint8_t er = 0u; er < NUM_ROOMS; er++)
            {
                if(env[ep][ew][er] == true)
                {
                    pP[ep]++;
                    pW[ew]++;
                    pR[er]++;
                }
            }
        }
    }
    for(uint8_t p = 0u; p < NUM_SUSPECTS; p++)
    {
        for(uint8_t w = 0u; w < NUM_WEAPONS; w++)
        {
            for(uint8_t r = 0u; r < NUM_ROOMS; r++)
            {
                if(reachableRooms[r] == false)
                {
                    continue;
                }

                float    h      = pP[p] + pW[w] + pR[r];
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
                        .suspect = p,
                        .weapon  = w,
                        .room    = r,
                        .h       = h
                    };
                }
            }
        }
    }

    return candidates;
}

//***************************************************************************//
//************************** PUBLIC FUNCTION DEFINITIONS *******************//
//***************************************************************************//

void recommender_getGuess(
    const GameSettings* settings,
    GameState*          game,
    const bool*         reachableRooms,
    uint8_t*            outP,
    uint8_t*            outW,
    uint8_t*            outR)
{
    float   minAvgEnvelopes = INFINITY;
    uint8_t minP            = 0;
    uint8_t minW            = 0;
    uint8_t minR            = 0;

    bool env[NUM_SUSPECTS][NUM_WEAPONS][NUM_ROOMS] = { false };
    buildEnvelopeArray(game, env);

    Guess* guesses = guessHeuristic(env, reachableRooms, NUM_HEURISTIC_GUESSES);

    for(uint32_t i = 0u; i < NUM_HEURISTIC_GUESSES; i++)
    {
        uint8_t p = guesses[i].suspect;
        uint8_t w = guesses[i].weapon;
        uint8_t r = guesses[i].room;

        uint32_t totalEnvelopes = 0u;
        uint32_t numPatterns    = 0u;
        for(uint8_t pat = 0u; pat < (1u << (settings->numPlayers - 1u)); pat++)
        {
            // Don't consider player, skip patterns where player answers yes
            if(((pat >> (settings->humanPlayer - 1u)) & 1) == 1u)
            {
                continue;
            }
            uint32_t numEnvelopes = countPossibleEnvelopesForGuessPattern(game, settings, settings->humanPlayer, env, p, w, r, pat);
            if(numEnvelopes == 0u) // Pattern is infeasible
            {
                continue;
            }
            numPatterns++;
            totalEnvelopes += numEnvelopes;
        }

        float avgEnvelopes = (float)totalEnvelopes / numPatterns; // numPatterns > 0

        if(avgEnvelopes < minAvgEnvelopes)
        {
            minP            = p;
            minW            = w;
            minR            = r;
            minAvgEnvelopes = avgEnvelopes;
        }
#if defined(DEBUG)
        printf("%s %s %s\n", cards_getSuspectName(p), cards_getWeaponName(w), cards_getRoomName(r));
        printf("%f (min: %f)\n", avgEnvelopes, minAvgEnvelopes);
#endif
    }

    free(guesses);
    *outP = minP;
    *outW = minW;
    *outR = minR;
}

bool recommender_getKeyGuess(GameState* game, const GameSettings* settings, uint8_t* outP, uint8_t* outC)
{
    bool env[NUM_SUSPECTS][NUM_WEAPONS][NUM_ROOMS] = { false };
    buildEnvelopeArray(game, env);

    // Try each card and player to find the best key guess
    float   minAvgEnvelopes = INFINITY;
    uint8_t bestPlayer      = 0;
    uint8_t bestCard        = 0;

    for(uint8_t c = 0u; c < NUM_CARDS; c++)
    {
        for(uint8_t pl = 1u; pl < settings->numPlayers; pl++)
        {
            if(kb_getAssignmentValue(game, c, pl) != SAT_UNKNOWN)
            {
                // Don't consider cards that are already known
                continue;
            }

            // Count envelopes if player has this card
            kb_pushLevel((GameState*)game);
            kb_addKeyGuessClauses(game, c, pl, true);
            uint32_t numEnvelopesYes = countPossibleEnvelopes((GameState*)game, env);
            kb_popLevel(game);

            // Count envelopes if player doesn't have this card
            kb_pushLevel((GameState*)game);
            kb_addKeyGuessClauses(game, c, pl, false);
            uint32_t numEnvelopesNo = countPossibleEnvelopes((GameState*)game, env);
            kb_popLevel(game);

#if defined(DEBUG)
            printf("Card %s, Player %s: Yes=%u, No=%u (curr. avg=%f)\n", cards_getCardName(c), settings->playerNames[pl], numEnvelopesYes, numEnvelopesNo, minAvgEnvelopes);
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
    for(uint8_t i = 0u; i < NUM_SUSPECTS; i++)
    {
        if(kb_getAssignmentValue(game, cards_getSuspectCard(i), ENVELOPE_PLAYER_ID) == SAT_TRUE)
        {
            *outP = i;
            found++;
            break;
        }
    }
    for(uint8_t i = 0u; i < NUM_WEAPONS; i++)
    {
        if(kb_getAssignmentValue(game, cards_getWeaponCard(i), ENVELOPE_PLAYER_ID) == SAT_TRUE)
        {
            *outW = i;
            found++;
            break;
        }
    }
    for(uint8_t i = 0u; i < NUM_ROOMS; i++)
    {
        if(kb_getAssignmentValue(game, cards_getRoomCard(i), ENVELOPE_PLAYER_ID) == SAT_TRUE)
        {
            *outR = i;
            found++;
            break;
        }
    }
    return (found == 3u);
}
