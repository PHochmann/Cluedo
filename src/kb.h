/**
 * @file kb.h
 */


//***************************************************************************//
//************************** INCLUDES ***************************************//
//***************************************************************************//

#pragma once
#include <stdint.h>
#include <stdbool.h>

#include "sat.h"

//***************************************************************************//
//**************************  PUBLIC TYPEDEFS *******************************//
//***************************************************************************//

/** @brief Todo */
#define MAX_NUM_PLAYERS 7u // Including envelope

/** @brief Todo */
#define NUM_PERSONS 6u // Only cards, i.e. without envelope

/** @brief Todo */
#define NUM_WEAPONS 6u

/** @brief Todo */
#define NUM_ROOMS 9u

/** @brief Todo */
#define NUM_CARDS (NUM_PERSONS + NUM_WEAPONS + NUM_ROOMS)

/** @brief Todo */
#define NUM_CARD_VARIABLES (MAX_NUM_PLAYERS * NUM_CARDS)

/** @brief Todo */
#define ENVELOPE_PLAYER_ID 0u

/** @brief Todo */
#define NUM_P_W_R_COMBINATONS (NUM_PERSONS * NUM_WEAPONS * NUM_ROOMS)

/** @brief Todo */
#define PERSON_CARD_OFFSET 0u

/** @brief Todo */
#define WEAPON_CARD_OFFSET (NUM_PERSONS)

/** @brief Todo */
#define ROOM_CARD_OFFSET (NUM_PERSONS + NUM_WEAPONS)

/**
 * @brief Todo
 * 
 * @var GameState::kb Todo
 * @var GameState::player Todo
 * @var GameState::numPlayers Todo
 * @var GameState::currAssignment Todo
 * @var GameState::numCards Todo
 * @var GameState::playerNames Todo
 */
typedef struct
{
    SAT_Problem* kb;
    uint8_t      player;
    uint8_t      numPlayers;
    int8_t*      currAssignment;
    uint8_t      numCards[MAX_NUM_PLAYERS];
    char*        playerNames[MAX_NUM_PLAYERS];
} GameState;

/**
 * @brief Todo
 * 
 * @var KB_ConflictInfo::numLiterals Todo
 * @var KB_ConflictInfo::literals Todo
 */
typedef struct
{
    uint32_t           numLiterals;
    const SAT_Literal* literals;
} KB_ConflictInfo;

//***************************************************************************//
//**************************  PUBLIC VARIABLE DECLARATIONS ******************//
//***************************************************************************//

/**
 * @brief Todo
 * 
 * @param card Todo
 * @param player Todo
 * @return Todo
 */
SAT_Literal kb_getLiteral(uint8_t card, uint8_t player);

/**
 * @brief Todo
 * 
 * @param card Todo
 * @param player Todo
 * @return Todo
 */
uint32_t kb_getVar(uint8_t card, uint8_t player);

/**
 * @brief Todo
 * 
 * @param personIndex Todo
 * @return Todo
 */
uint8_t kb_getPersonCard(uint8_t personIndex);

/**
 * @brief Todo
 * 
 * @param weaponIndex Todo
 * @return Todo
 */
uint8_t kb_getWeaponCard(uint8_t weaponIndex);

/**
 * @brief Todo
 * 
 * @param roomIndex Todo
 * @return Todo
 */
uint8_t kb_getRoomCard(uint8_t roomIndex);

/**
 * @brief Todo
 * 
 * @param game Todo
 */
void kb_addRulesetClauses(GameState* game);

/**
 * @brief Todo
 * 
 * @param game Todo
 * @param p Todo
 * @param w Todo
 * @param r Todo
 * @param answeringPlayer Todo
 * @param answer Todo
 */
void kb_addGuessAnswerClauses(GameState* game, uint8_t p, uint8_t w, uint8_t r, uint8_t answeringPlayer, int8_t answer);

/**
 * @brief Todo
 * 
 * @param game Todo
 * @param answeringPlayer Todo
 * @param card Todo
 * @param answer Todo
 */
void kb_addKeyGuessClauses(GameState* game, uint8_t answeringPlayer, uint8_t card, int8_t answer);

/**
 * @brief Todo
 * 
 * @param game Todo
 * @param p Todo
 * @param w Todo
 * @param r Todo
 */
void kb_addFailedAccusationClauses(GameState* game, uint8_t p, uint8_t w, uint8_t r);

/**
 * @brief Todo
 * 
 * @param game Todo
 * @param player Todo
 * @param numCards Todo
 * @param cards Todo
 */
void kb_addStartCardClauses(GameState* game, uint8_t player, uint8_t numCards, int8_t* cards);

/**
 * @brief Todo
 * 
 * @param game Todo
 * @param outConflict Todo
 * @return Todo
 */
bool kb_updateAssignment(GameState* game, KB_ConflictInfo* outConflict);
