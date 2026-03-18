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

#include "kb.h"
#include "sat.h"

//***************************************************************************//
//************************** PRIVATE FUNCTION DECLARATIONS ******************//
//***************************************************************************//

/**
 * @brief Todo
 * 
 * @param baseVar Todo
 * @param i Todo
 * @param j Todo
 * @param k Todo
 * @return Todo
 */
static SAT_Literal getCounterAuxLit(uint32_t baseVar, uint8_t i, uint8_t j, uint8_t k);

/**
 * @brief Todo
 * 
 * @param game Todo
 * @param playerIndex Todo
 * @param cardsOfPlayer Todo
 * @param baseVar Todo
 * @param sign Todo
 */
static void encodeAtMostKCards(const GameState* game, uint8_t playerIndex, uint8_t cardsOfPlayer, uint32_t baseVar, int8_t sign);

//***************************************************************************//
//************************** PRIVATE FUNCTION DEFINITIONS *******************//
//***************************************************************************//

static SAT_Literal getCounterAuxLit(uint32_t baseVar, uint8_t i, uint8_t j, uint8_t k)
{
    return baseVar + (i * k) + j;
}

static void encodeAtMostKCards(const GameState* game, uint8_t playerIndex, uint8_t k, uint32_t baseVar, int8_t sign)
{
    for(uint8_t i = 0u; i < NUM_CARDS; i++)
    {
        SAT_Literal xi = kb_getLiteral(i, playerIndex) * sign;

        for(uint8_t j = 0u; (j < k) && (j <= i); j++)
        {
            SAT_Literal sij = getCounterAuxLit(baseVar, i, j, k);

            /* 1. xi → s(i,0) */
            if(j == 0u)
            {
                sat_addClause(game->kb, 2u, -xi, sij);
            }

            /* 2. s(i-1,j) → s(i,j) */
            if(i > 0u)
            {
                sat_addClause(game->kb, 2u, -getCounterAuxLit(baseVar, i - 1u, j, k), sij);
            }

            /* 3. xi ∧ s(i-1,j-1) → s(i,j) */
            if((i > 0u) && (j > 0u))
            {
                sat_addClause(game->kb, 3u, -xi, -getCounterAuxLit(baseVar, i - 1u, j - 1u, k), sij);
            }
        }

        /* 4. forbid overflow: xi → -s(i-1,k-1) */
        if(i > 0u)
        {
            sat_addClause(game->kb, 2u, -xi, -getCounterAuxLit(baseVar, i - 1u, k - 1u, k));
        }
    }
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

uint8_t kb_getPersonCard(uint8_t personIndex)
{
    return personIndex + PERSON_CARD_OFFSET;
}

uint8_t kb_getWeaponCard(uint8_t weaponIndex)
{
    return weaponIndex + WEAPON_CARD_OFFSET;
}

uint8_t kb_getRoomCard(uint8_t roomIndex)
{
    return roomIndex + ROOM_CARD_OFFSET;
}

void kb_addRulesetClauses(GameState* game)
{
    uint32_t numAuxVars = (game->numPlayers - 1u) * NUM_CARDS * NUM_CARDS;
    game->kb            = sat_initProblem(NUM_CARD_VARIABLES + numAuxVars);
    SAT_Problem* kb     = game->kb;

    // 1. Each card belongs to exactly one player (including envelope)
    // 1.1 Each card belongs to at least one player
    for(uint8_t card = 0u; card < NUM_CARDS; card++)
    {
        SAT_Literal cardPlayerCombinations[MAX_NUM_PLAYERS];
        for(uint8_t player = 0u; player < game->numPlayers; player++)
        {
            cardPlayerCombinations[player] = kb_getLiteral(card, player);
        }
        sat_vAddClause(kb, game->numPlayers, cardPlayerCombinations);
    }
    // 1.2 Each card belongs to at most one player
    for(uint8_t card = 0u; card < NUM_CARDS; card++)
    {
        for(uint8_t playerA = 0u; playerA < game->numPlayers; playerA++)
        {
            for(uint8_t playerB = playerA + 1u; playerB < game->numPlayers; playerB++)
            {
                SAT_Literal playerCombination[] = {
                    -kb_getLiteral(card, playerA),
                    -kb_getLiteral(card, playerB)
                };
                sat_vAddClause(kb, 2u, playerCombination);
            }
        }
    }

    // 2. Envelope
    // 2.1 Envelope has one of each kind
    SAT_Literal personEnvelopePairs[NUM_PERSONS];
    for(uint8_t person = 0u; person < NUM_PERSONS; person++)
    {
        personEnvelopePairs[person] = kb_getLiteral(kb_getPersonCard(person), ENVELOPE_PLAYER_ID);
    }
    sat_vAddClause(kb, NUM_PERSONS, personEnvelopePairs);
    SAT_Literal weaponEnvelopePairs[NUM_WEAPONS];
    for(uint8_t weapon = 0u; weapon < NUM_WEAPONS; weapon++)
    {
        weaponEnvelopePairs[weapon] = kb_getLiteral(kb_getWeaponCard(weapon), ENVELOPE_PLAYER_ID);
    }
    sat_vAddClause(kb, NUM_WEAPONS, weaponEnvelopePairs);
    SAT_Literal roomEnvelopePairs[NUM_ROOMS];
    for(uint8_t room = 0u; room < NUM_ROOMS; room++)
    {
        roomEnvelopePairs[room] = kb_getLiteral(kb_getRoomCard(room), ENVELOPE_PLAYER_ID);
    }
    sat_vAddClause(kb, NUM_ROOMS, roomEnvelopePairs);
    // 2.2 Envelope does not have two cards of the same kind
    for(uint8_t personA = 0u; personA < NUM_PERSONS; personA++)
    {
        for(uint8_t personB = personA + 1u; personB < NUM_PERSONS; personB++)
        {
            SAT_Literal personCombination[] = {
                -kb_getLiteral(kb_getPersonCard(personA), ENVELOPE_PLAYER_ID),
                -kb_getLiteral(kb_getPersonCard(personB), ENVELOPE_PLAYER_ID)
            };
            sat_vAddClause(kb, 2u, personCombination);
        }
    }
    for(uint8_t weaponA = 0u; weaponA < NUM_WEAPONS; weaponA++)
    {
        for(uint8_t weaponB = weaponA + 1u; weaponB < NUM_WEAPONS; weaponB++)
        {
            SAT_Literal weaponCombination[] = {
                -kb_getLiteral(kb_getWeaponCard(weaponA), ENVELOPE_PLAYER_ID),
                -kb_getLiteral(kb_getWeaponCard(weaponB), ENVELOPE_PLAYER_ID)
            };
            sat_vAddClause(kb, 2u, weaponCombination);
        }
    }
    for(uint8_t roomA = 0u; roomA < NUM_ROOMS; roomA++)
    {
        for(uint8_t roomB = roomA + 1u; roomB < NUM_ROOMS; roomB++)
        {
            SAT_Literal roomCombination[] = {
                -kb_getLiteral(kb_getRoomCard(roomA), ENVELOPE_PLAYER_ID),
                -kb_getLiteral(kb_getRoomCard(roomB), ENVELOPE_PLAYER_ID)
            };
            sat_vAddClause(kb, 2u, roomCombination);
        }
    }

    // 3. Encode number of cards per player
    uint32_t base = NUM_CARD_VARIABLES + 1u;
    for(uint8_t playerIndex = 1u; playerIndex < game->numPlayers; playerIndex++)
    {
        encodeAtMostKCards(game, playerIndex, game->numCards[playerIndex], base, 1);
        base += game->numCards[playerIndex] * (NUM_CARDS);
        encodeAtMostKCards(game, playerIndex, NUM_CARDS - game->numCards[playerIndex], base, -1);
        base += (NUM_CARDS - game->numCards[playerIndex]) * (NUM_CARDS);
    }

#ifdef DEBUG
    printf("Knowledge base initialized with %u variables and %lu clauses.\n", sat_getNumVariables(kb), sat_getNumClauses(kb));
#endif
}

void kb_addGuessAnswerClauses(GameState* game, uint8_t p, uint8_t w, uint8_t r, uint8_t answeringPlayer, int8_t answer)
{
    SAT_Literal pLit = kb_getLiteral(kb_getPersonCard(p), answeringPlayer);
    SAT_Literal wLit = kb_getLiteral(kb_getWeaponCard(w), answeringPlayer);
    SAT_Literal rLit = kb_getLiteral(kb_getRoomCard(r), answeringPlayer);
    if(answer == 0)
    {
        sat_addUnitClause(game->kb, -pLit);
        sat_addUnitClause(game->kb, -wLit);
        sat_addUnitClause(game->kb, -rLit);
    }
    else
    {
        sat_addClause(game->kb, 3u, pLit, wLit, rLit);
    }
}

void kb_addKeyGuessClauses(GameState* game, uint8_t answeringPlayer, uint8_t card, int8_t answer)
{
    if(answer == 1u)
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
    SAT_Literal pLit = kb_getLiteral(kb_getPersonCard(p), ENVELOPE_PLAYER_ID);
    SAT_Literal wLit = kb_getLiteral(kb_getWeaponCard(w), ENVELOPE_PLAYER_ID);
    SAT_Literal rLit = kb_getLiteral(kb_getRoomCard(r), ENVELOPE_PLAYER_ID);
    sat_addClause(game->kb, 3u, -pLit, -wLit, -rLit);
}

void kb_addStartCardClauses(GameState* game, uint8_t player, uint8_t numCards, int8_t* cards)
{
    for(uint8_t i = 0; i < numCards; i++)
    {
        sat_addUnitClause(game->kb, kb_getLiteral(cards[i], player));
    }
}

bool kb_updateAssignment(GameState* game, KB_ConflictInfo* outConflict)
{
    uint32_t           numConflictLits;
    const SAT_Literal* conflictClause;
    bool*              solution;

    if(sat_isSatisfiable(game->kb, &solution, &numConflictLits, &conflictClause) == false)
    {
        if(outConflict != NULL)
        {
            outConflict->numLiterals = numConflictLits;
            outConflict->literals    = conflictClause;
        }
        return false;
    }

    for(uint32_t i = 0u; i < NUM_CARD_VARIABLES; i++)
    {
        game->currAssignment[i] = sat_getBackboneValue(game->kb, solution, i);
    }
    free(solution);
    return true;
}
