/**
 * @file kb.h
 */

 
#pragma once

//***************************************************************************//
//************************** INCLUDES ***************************************//
//***************************************************************************//

#include <stdint.h>
#include <stdbool.h>

#include "sat.h"

//***************************************************************************//
//**************************  PUBLIC TYPEDEFS *******************************//
//***************************************************************************//

/** @brief Maximum number of players, including the envelope pseudo-player. */
#define MAX_NUM_PLAYERS 7u

/** @brief Number of person cards in the game. */
#define NUM_PERSONS 6u

/** @brief Number of weapon cards in the game. */
#define NUM_WEAPONS 6u

/** @brief Number of room cards in the game. */
#define NUM_ROOMS 9u

/** @brief Total number of cards in the game. */
#define NUM_CARDS (NUM_PERSONS + NUM_WEAPONS + NUM_ROOMS)

/** @brief Total number of SAT variables for card-player assignments. */
#define NUM_CARD_VARIABLES (MAX_NUM_PLAYERS * NUM_CARDS)

/** @brief Player ID representing the envelope (the solution). */
#define ENVELOPE_PLAYER_ID 0u

/** @brief Index offset for person cards within the card array. */
#define PERSON_CARD_OFFSET 0u

/** @brief Index offset for weapon cards within the card array. */
#define WEAPON_CARD_OFFSET (NUM_PERSONS)

/** @brief Index offset for room cards within the card array. */
#define ROOM_CARD_OFFSET (NUM_PERSONS + NUM_WEAPONS)

/**
 * @brief Holds the complete game state, including the SAT knowledge base and player information.
 * 
 * @var GameState::kb The SAT problem representing the knowledge base.
 * @var GameState::player Index of the local (human) player.
 * @var GameState::numPlayers Total number of players, including the envelope pseudo-player.
 * @var GameState::currAssignment Current backbone assignment for all card-player variables (-1, 0, or 1).
 * @var GameState::numCards Number of cards held by each player.
 * @var GameState::playerNames Display names of all players.
 */
typedef struct
{
    SAT_Problem* kb;
    uint8_t      player;
    uint8_t      numPlayers;
    int8_t*      currAssignment;
    bool*      positiveAnswers;
    uint8_t      numCards[MAX_NUM_PLAYERS];
    char*        playerNames[MAX_NUM_PLAYERS];
} GameState;

/**
 * @brief Holds conflict information returned by the SAT solver when the knowledge base is unsatisfiable.
 * 
 * @var KB_ConflictInfo::numLiterals Number of literals in the conflict clause.
 * @var KB_ConflictInfo::literals Pointer to the array of conflict clause literals.
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
 * @brief Returns the positive SAT literal for the assignment of a card to a player.
 * 
 * @param card Card index (0..NUM_CARDS-1).
 * @param player Player index (0..numPlayers-1).
 * @return 1-based positive SAT literal.
 */
SAT_Literal kb_getLiteral(uint8_t card, uint8_t player);

/**
 * @brief Returns the 0-based SAT variable index for the assignment of a card to a player.
 * 
 * @param card Card index (0..NUM_CARDS-1).
 * @param player Player index (0..numPlayers-1).
 * @return 0-based SAT variable index.
 */
uint32_t kb_getVar(uint8_t card, uint8_t player);

/**
 * @brief Returns the card index for a person card given its person index.
 * 
 * @param personIndex Index of the person (0..NUM_PERSONS-1).
 * @return Card index of the person card.
 */
uint8_t kb_getPersonCard(uint8_t personIndex);

/**
 * @brief Returns the card index for a weapon card given its weapon index.
 * 
 * @param weaponIndex Index of the weapon (0..NUM_WEAPONS-1).
 * @return Card index of the weapon card.
 */
uint8_t kb_getWeaponCard(uint8_t weaponIndex);

/**
 * @brief Returns the card index for a room card given its room index.
 * 
 * @param roomIndex Index of the room (0..NUM_ROOMS-1).
 * @return Card index of the room card.
 */
uint8_t kb_getRoomCard(uint8_t roomIndex);

/**
 * @brief Initializes the SAT knowledge base and adds all Cluedo game-rule clauses.
 * 
 * @param game Current game state (kb will be allocated and populated).
 */
void kb_addRulesetClauses(GameState* game);

/**
 * @brief Adds clauses encoding the result of a guess (suggestion) round.
 * 
 * @param game Current game state.
 * @param p Index of the guessed person card.
 * @param w Index of the guessed weapon card.
 * @param r Index of the guessed room card.
 * @param answeringPlayer Index of the player who answered.
 * @param answer 1 if the answering player showed a card, 0 if they could not.
 */
void kb_addGuessAnswerClauses(GameState* game, uint8_t p, uint8_t w, uint8_t r, uint8_t answeringPlayer, int8_t answer);

/**
 * @brief Adds a unit clause encoding whether a specific player holds a specific card.
 * 
 * @param game Current game state.
 * @param answeringPlayer Index of the player who was asked.
 * @param card Card index that was asked about.
 * @param answer 1 if the player holds the card, 0 otherwise.
 */
void kb_addKeyGuessClauses(GameState* game, uint8_t answeringPlayer, uint8_t card, int8_t answer);

/**
 * @brief Adds a clause ruling out the given combination as the envelope solution after a failed accusation.
 * 
 * @param game Current game state.
 * @param p Index of the accused person card.
 * @param w Index of the accused weapon card.
 * @param r Index of the accused room card.
 */
void kb_addFailedAccusationClauses(GameState* game, uint8_t p, uint8_t w, uint8_t r);

/**
 * @brief Adds unit clauses asserting the cards dealt to a player at game start.
 * 
 * @param game Current game state.
 * @param player Index of the player who received the cards.
 * @param numCards Number of cards dealt to the player.
 * @param cards Array of card indices dealt to the player.
 */
void kb_addStartCardClauses(GameState* game, uint8_t player, uint8_t numCards, int8_t* cards);

/**
 * @brief Runs the SAT solver and updates the backbone assignment for all card-player variables.
 * 
 * @param game Current game state (currAssignment is updated on success).
 * @param outConflict If non-NULL, receives conflict clause information when the KB is unsatisfiable.
 * @return true if the knowledge base is satisfiable, false otherwise.
 */
bool kb_updateAssignment(GameState* game, KB_ConflictInfo* outConflict);

void kb_newGame(GameState* game);

void kb_freeGame(GameState* game);
