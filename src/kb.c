/**
 * @file kb.c
 */


//***************************************************************************//
//************************** INCLUDES ***************************************//
//***************************************************************************//

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "cards.h"
#include "kb.h"
#include "sat.h"

//***************************************************************************//
//************************** PRIVATE FUNCTION DECLARATIONS ******************//
//***************************************************************************//

/**
 * @brief Internal knowledge-base state for one game instance.
 *
 * @var GameState::kb SAT problem that stores all clauses.
 * @var GameState::currAssignment Cached backbone assignment for card-player variables.
 * @var GameState::positiveAnswers Marks cells that came from positive guess answers.
 */
struct GameState
{
    SAT_Problem*   kb;
    SAT_Assignment currAssignment[NUM_CARD_VARIABLES];
    bool           positiveAnswers[NUM_CARD_VARIABLES];
};

/**
 * @brief Gets the auxiliary SAT literal for the sequential-counter position (i, j).
 * 
 * @param baseVar Base variable index for auxiliary counter variables.
 * @param i Card index within the counter.
 * @param j Counter position (0-based).
 * @param k Maximum count (number of cards for the player).
 * @return SAT literal for the auxiliary counter variable at (i, j).
 */
static SAT_Literal getCounterAuxLit(uint32_t baseVar, uint32_t i, uint32_t j, uint32_t k);

/**
 * @brief Adds exactly-one encoding (at-least-one and pairwise at-most-one).
 *
 * @param kb SAT problem.
 * @param numLiterals Number of literals.
 * @param literals Literal array.
 */
static void addExactlyOneClauses(SAT_Problem* kb, uint32_t numLiterals, const SAT_Literal* literals);

/**
 * @brief Adds an at-most-k cardinality constraint with Sinz 2005 sequential counter encoding.
 *
 * @param kb SAT problem.
 * @param numLiterals Number of literals n.
 * @param literals Literal array.
 * @param k Maximum number of true literals allowed.
 * @param baseVar First auxiliary variable (1-based SAT variable index).
 * @return Number of auxiliary variables consumed.
 */
static uint32_t addAtMostKClauses(SAT_Problem* kb, uint32_t numLiterals, const SAT_Literal* literals, uint32_t k, uint32_t baseVar);

/**
 * @brief Adds an exact-k cardinality constraint over arbitrary literals.
 *
 * Implements exactly-k as at-most-k over the given literals and
 * at-most-(n-k) over their negations.
 *
 * @param kb SAT problem.
 * @param numLiterals Number of literals n.
 * @param literals Literal array.
 * @param k Required number of true literals.
 * @param baseVar First auxiliary variable (1-based SAT variable index).
 * @return Number of auxiliary variables consumed.
 */
static uint32_t addExactlyKClauses(SAT_Problem* kb, uint32_t numLiterals, const SAT_Literal* literals, uint32_t k, uint32_t baseVar);

/**
 * @brief Initializes the SAT knowledge base and adds all Cluedo game-rule clauses.
 *
 * @param settings Game settings to determine the number of players.
 * @return Initialized SAT problem instance.
 */
static SAT_Problem* initSatProblem(const GameSettings* settings);

//***************************************************************************//
//************************** PRIVATE FUNCTION DEFINITIONS *******************//
//***************************************************************************//

//***************************************************************************//
//**************************    CARDINALITY CONSTRAINTS   *******************//
//***************************************************************************//

static SAT_Literal getCounterAuxLit(uint32_t baseVar, uint32_t i, uint32_t j, uint32_t k)
{
    return baseVar + (i * k) + j;
}

static uint32_t addAtMostKClauses(SAT_Problem* kb, uint32_t numLiterals, const SAT_Literal* literals, uint32_t k, uint32_t baseVar)
{
    for(uint32_t i = 0u; i < numLiterals; i++)
    {
        SAT_Literal xi = literals[i];

        for(uint32_t j = 0u; (j < k) && (j <= i); j++)
        {
            SAT_Literal sij = getCounterAuxLit(baseVar, i, j, k);

            /* 1. xi → s(i,0) */
            if(j == 0u)
            {
                sat_addClause(kb, 2u, -xi, sij);
            }

            /* 2. s(i-1,j) → s(i,j) */
            if(i > 0u)
            {
                sat_addClause(kb, 2u, -getCounterAuxLit(baseVar, i - 1u, j, k), sij);
            }

            /* 3. xi ∧ s(i-1,j-1) → s(i,j) */
            if((i > 0u) && (j > 0u))
            {
                sat_addClause(kb, 3u, -xi, -getCounterAuxLit(baseVar, i - 1u, j - 1u, k), sij);
            }
        }

        /* 4. forbid overflow: xi → -s(i-1,k-1) */
        if(i > 0u)
        {
            sat_addClause(kb, 2u, -xi, -getCounterAuxLit(baseVar, i - 1u, k - 1u, k));
        }
    }

    return numLiterals * k;
}

static uint32_t addExactlyKClauses(SAT_Problem* kb, uint32_t numLiterals, const SAT_Literal* literals, uint32_t k, uint32_t baseVar)
{
    assert(k <= numLiterals);

    uint32_t usedAuxVars = 0u;
    usedAuxVars += addAtMostKClauses(kb, numLiterals, literals, k, baseVar + usedAuxVars);

    SAT_Literal* negatedLiterals = malloc(numLiterals * sizeof(SAT_Literal));
    for(uint32_t i = 0u; i < numLiterals; i++)
    {
        negatedLiterals[i] = -literals[i];
    }

    usedAuxVars += addAtMostKClauses(kb, numLiterals, negatedLiterals, numLiterals - k, baseVar + usedAuxVars);
    free(negatedLiterals);
    return usedAuxVars;
}

static void addExactlyOneClauses(SAT_Problem* kb, uint32_t numLiterals, const SAT_Literal* literals)
{
    // At least one
    sat_vAddClause(kb, numLiterals, literals);

    // At most one (pairwise)
    for(uint32_t i = 0u; i < numLiterals; i++)
    {
        for(uint32_t j = i + 1u; j < numLiterals; j++)
        {
            sat_addClause(kb, 2u, -literals[i], -literals[j]);
        }
    }
}

//***************************************************************************//
//**************************    INITIAL KNOWLEDGE BASE    *******************//
//***************************************************************************//

static SAT_Problem* initSatProblem(const GameSettings* settings)
{
    const uint32_t numAuxVars = (settings->numPlayers - 1u) * NUM_CARDS * NUM_CARDS;
    SAT_Problem*   kb         = sat_initProblem(NUM_CARD_VARIABLES + numAuxVars);

    // 1. Each card belongs to exactly one player (including envelope)
    for(uint8_t card = 0u; card < NUM_CARDS; card++)
    {
        SAT_Literal cardPlayerCombinations[MAX_NUM_PLAYERS];
        for(uint8_t player = 0u; player < settings->numPlayers; player++)
        {
            cardPlayerCombinations[player] = kb_getLiteral(card, player);
        }
        addExactlyOneClauses(kb, settings->numPlayers, cardPlayerCombinations);
    }

    // 2. Envelope has exactly one card of each kind
    SAT_Literal suspectEnvelopePairs[NUM_SUSPECTS];
    for(uint8_t suspect = 0u; suspect < NUM_SUSPECTS; suspect++)
    {
        suspectEnvelopePairs[suspect] = kb_getLiteral(cards_getSuspectCard(suspect), ENVELOPE_PLAYER_ID);
    }
    addExactlyOneClauses(kb, NUM_SUSPECTS, suspectEnvelopePairs);

    SAT_Literal weaponEnvelopePairs[NUM_WEAPONS];
    for(uint8_t weapon = 0u; weapon < NUM_WEAPONS; weapon++)
    {
        weaponEnvelopePairs[weapon] = kb_getLiteral(cards_getWeaponCard(weapon), ENVELOPE_PLAYER_ID);
    }
    addExactlyOneClauses(kb, NUM_WEAPONS, weaponEnvelopePairs);

    SAT_Literal roomEnvelopePairs[NUM_ROOMS];
    for(uint8_t room = 0u; room < NUM_ROOMS; room++)
    {
        roomEnvelopePairs[room] = kb_getLiteral(cards_getRoomCard(room), ENVELOPE_PLAYER_ID);
    }
    addExactlyOneClauses(kb, NUM_ROOMS, roomEnvelopePairs);

    // 3. Encode number of cards per player
    uint32_t base = NUM_CARD_VARIABLES + 1u;
    for(uint8_t playerIndex = 1u; playerIndex < settings->numPlayers; playerIndex++)
    {
        SAT_Literal playerCardLiterals[NUM_CARDS];
        for(uint8_t card = 0u; card < NUM_CARDS; card++)
        {
            playerCardLiterals[card] = kb_getLiteral(card, playerIndex);
        }
        base += addExactlyKClauses(kb, NUM_CARDS, playerCardLiterals, settings->numCards[playerIndex], base);
    }

#ifdef DEBUG
    printf("Knowledge base initialized with %u variables and %zu clauses.\n", sat_getNumVariables(kb), sat_getNumClauses(kb));
#endif
    return kb;
}

//***************************************************************************//
//**************************  PUBLIC FUNCTION DEFINITIONS *******************//
//***************************************************************************//

SAT_Literal kb_getLiteral(uint8_t card, uint8_t player)
{
    return NUM_CARDS * player + card + 1;
}

uint32_t kb_getVar(uint8_t card, uint8_t player)
{
    return NUM_CARDS * player + card;
}

void kb_addGuessAnswerClauses(GameState* game, uint8_t p, uint8_t w, uint8_t r, uint8_t answeringPlayer, bool answer, bool updateHintMarks)
{
    SAT_Literal pLit = kb_getLiteral(cards_getSuspectCard(p), answeringPlayer);
    SAT_Literal wLit = kb_getLiteral(cards_getWeaponCard(w), answeringPlayer);
    SAT_Literal rLit = kb_getLiteral(cards_getRoomCard(r), answeringPlayer);

    if(answer == true)
    {
        sat_addClause(game->kb, 3u, pLit, wLit, rLit);
    }
    else
    {
        sat_addUnitClause(game->kb, -pLit);
        sat_addUnitClause(game->kb, -wLit);
        sat_addUnitClause(game->kb, -rLit);
    }

    if((answer == true) && (updateHintMarks == true))
    {
        game->positiveAnswers[kb_getVar(cards_getSuspectCard(p), answeringPlayer)] = true;
        game->positiveAnswers[kb_getVar(cards_getWeaponCard(w), answeringPlayer)]  = true;
        game->positiveAnswers[kb_getVar(cards_getRoomCard(r), answeringPlayer)]    = true;
    }
}

void kb_addKeyGuessClauses(GameState* game, uint8_t answeringPlayer, uint8_t card, bool answer)
{
    if(answer == true)
    {
        sat_addUnitClause(game->kb, kb_getLiteral(card, answeringPlayer));
    }
    else
    {
        sat_addUnitClause(game->kb, -kb_getLiteral(card, answeringPlayer));
    }
}

void kb_addFailedAccusationClauses(GameState* game, uint8_t p, uint8_t w, uint8_t r)
{
    SAT_Literal pLit = kb_getLiteral(cards_getSuspectCard(p), ENVELOPE_PLAYER_ID);
    SAT_Literal wLit = kb_getLiteral(cards_getWeaponCard(w), ENVELOPE_PLAYER_ID);
    SAT_Literal rLit = kb_getLiteral(cards_getRoomCard(r), ENVELOPE_PLAYER_ID);
    sat_addClause(game->kb, 3u, -pLit, -wLit, -rLit);
}

void kb_addStartCardClauses(GameState* game, const GameSettings* settings, uint8_t player, const int8_t* cards)
{
    uint8_t numCards = settings->numCards[player];

    for(uint8_t i = 0u; i < numCards; i++)
    {
        sat_addUnitClause(game->kb, kb_getLiteral(cards[i], player));
    }
}

SAT_Assignment kb_getAssignmentValue(const GameState* game, uint8_t card, uint8_t player)
{
    return game->currAssignment[kb_getVar(card, player)];
}

bool kb_getPositiveAnswerMark(const GameState* game, uint8_t card, uint8_t player)
{
    return game->positiveAnswers[kb_getVar(card, player)];
}

bool kb_updateAssignment(GameState* game)
{
    const SAT_Result* result;
    if(sat_isSatisfiable(game->kb, &result) == false)
    {
        sat_freeResult(result);
        return false;
    }
    for(uint32_t i = 0u; i < NUM_CARD_VARIABLES; i++)
    {
        if(game->currAssignment[i] == SAT_UNKNOWN)
        {
            game->currAssignment[i] = sat_getBackboneValue(game->kb, result->solution, i);
        }
    }
    sat_freeResult(result);
    return true;
}

GameState* kb_newGame(const GameSettings* settings)
{
    GameState* game = malloc(sizeof(GameState));
    *game           = (GameState){
                  .kb              = initSatProblem(settings),
                  .currAssignment  = { 0 },
                  .positiveAnswers = { false }
    };

    // Sanity check
    if(sat_isSatisfiable(game->kb, NULL) == false)
    {
        printf("Software defect - logical error in initial knowledge base\n");
        kb_freeGame(game);
        exit(1);
    }

    return game;
}

void kb_freeGame(GameState* game)
{
    sat_freeProblem(game->kb);
    free(game);
}

//***************************************************************************//
//**************************  SAT PROBLEM  **********************************//
//***************************************************************************//

void kb_pushLevel(GameState* game)
{
    sat_pushLevel(game->kb);
}

void kb_popLevel(GameState* game)
{
    sat_popLevel(game->kb);
}

bool kb_isSatisfiable(const GameState* game)
{
    return sat_isSatisfiable(game->kb, NULL);
}

bool kb_isSatisfiableWithAssumptions(const GameState* game, uint32_t numAssumptions, const SAT_Literal* assumptions)
{
    return sat_isSatisfiableWithAssumptions(game->kb, numAssumptions, assumptions, NULL);
}
