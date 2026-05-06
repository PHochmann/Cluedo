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
//**************************         PUBLIC DEFINES        ******************//
//***************************************************************************//

/** @brief Maximum number of players, including the envelope pseudo-player. */
#define MAX_NUM_PLAYERS 7u

/** @brief Number of suspect cards in the game. */
#define NUM_SUSPECTS 6u

/** @brief Number of weapon cards in the game. */
#define NUM_WEAPONS 6u

/** @brief Number of room cards in the game. */
#define NUM_ROOMS 9u

/** @brief Total number of cards in the game. */
#define NUM_CARDS (NUM_SUSPECTS + NUM_WEAPONS + NUM_ROOMS)

/** @brief Total number of SAT variables for card-player assignments. */
#define NUM_CARD_VARIABLES (MAX_NUM_PLAYERS * NUM_CARDS)

/** @brief Player ID representing the envelope (the solution). */
#define ENVELOPE_PLAYER_ID 0u

//***************************************************************************//
//**************************  PUBLIC TYPEDEFS *******************************//
//***************************************************************************//

/**
 * @brief Stores configurable player metadata for a single game.
 *
 * @var GameSettings::humanPlayer Index of the local (human) player.
 * @var GameSettings::numPlayers Total number of players, including the envelope pseudo-player.
 * @var GameSettings::numCards Number of cards held by each player.
 * @var GameSettings::playerNames Display names of all players.
 */
typedef struct
{
    uint8_t humanPlayer;
    uint8_t numPlayers;
    uint8_t numCards[MAX_NUM_PLAYERS];
    char*   playerNames[MAX_NUM_PLAYERS];
} GameSettings;

/**
 * @brief Opaque game state handle used by the knowledge base module.
 * 
 * Internal SAT data structures are hidden in kb.c.
 */
typedef struct GameState GameState;

//***************************************************************************//
//**************************  PUBLIC VARIABLE DECLARATIONS ******************//
//***************************************************************************//

//***************************************************************************//
//**************************           GAME STATE          ******************//
//***************************************************************************//

/**
 * @brief Runs the SAT solver and updates the backbone assignment for all card-player variables.
 * 
 * @param game Current game state (currAssignment is updated on success).
 * @return true if the knowledge base is satisfiable, false otherwise.
 */
bool kb_updateAssignment(GameState* game);

/**
 * @brief Allocates and initializes a new game from provided settings and adds all game-rule clauses.
 *
 * @param settings Pre-built game settings from main setup.
 * @return Heap-allocated game state.
 */
GameState* kb_newGame(const GameSettings* settings);

/**
 * @brief Frees all resources of a game created by kb_newGame(), including the GameState object itself.
 *
 * @param game Game state to free.
 */
void kb_freeGame(GameState* game);

//***************************************************************************//
//**************************   LITERALS AND CARD MAPPINGS  ******************//
//***************************************************************************//

/**
 * @brief Returns the positive SAT literal for the assignment of a card to a player.
 * 
 * @param card Card index (0..NUM_CARDS-1).
 * @param player Player index (0..numPlayers-1).
 * @return 1-based positive SAT literal.
 */
SAT_Literal kb_getLiteral(uint8_t card, uint8_t player);

//***************************************************************************//
//**************************        CLAUSE INSERTION       ******************//
//***************************************************************************//

/**
 * @brief Adds clauses encoding the result of a guess (suggestion) round.
 * 
 * @param game Current game state.
 * @param p Index of the guessed suspect card.
 * @param w Index of the guessed weapon card.
 * @param r Index of the guessed room card.
 * @param answeringPlayer Index of the player who answered.
 * @param answer true if the answering player showed a card, false if they could not.
 * @param updateHintMarks true to update positive-answer marks, false to skip mark updates.
 *               When answer is true, positive-answer marks are updated for sheet hints.
 */
void kb_addGuessAnswerClauses(GameState* game, uint8_t p, uint8_t w, uint8_t r, uint8_t answeringPlayer, bool answer, bool updateHintMarks);

/**
 * @brief Adds a unit clause encoding whether a specific player holds a specific card.
 * 
 * @param game Current game state.
 * @param answeringPlayer Index of the player who was asked.
 * @param card Card index that was asked about.
 * @param answer true if the player holds the card, false otherwise.
 */
void kb_addKeyGuessClauses(GameState* game, uint8_t answeringPlayer, uint8_t card, bool answer);

/**
 * @brief Adds a clause ruling out the given combination as the envelope solution after a failed accusation.
 * 
 * @param game Current game state.
 * @param p Index of the accused suspect card.
 * @param w Index of the accused weapon card.
 * @param r Index of the accused room card.
 */
void kb_addFailedAccusationClauses(GameState* game, uint8_t p, uint8_t w, uint8_t r);

/**
 * @brief Adds unit clauses asserting the cards dealt to a player at game start.
 * 
 * @param game Current game state.
 * @param settings Immutable game settings.
 * @param player Player who receives the cards.
 * @param cards Array of card indices dealt to the player.
 */
void kb_addStartCardClauses(GameState* game, const GameSettings* settings, uint8_t player, const int8_t* cards);

//***************************************************************************//
//**************************        ASSIGNMENT QUERY       ******************//
//***************************************************************************//

/**
 * @brief Reads the current assignment value for one card/player pair.
 *
 * @param game Current game state.
 * @param card Card index.
 * @param player Player index.
 * @return SAT_TRUE if the player holds the card, SAT_FALSE if the player does not hold the card, SAT_UNKNOWN if unknown.
 */
SAT_Assignment kb_getAssignmentValue(const GameState* game, uint8_t card, uint8_t player);

/**
 * @brief Returns whether a cell is marked as a positive-answer hint.
 *
 * @param game Current game state.
 * @param card Card index.
 * @param player Player index.
 * @return true if marked, false otherwise.
 */
bool kb_getPositiveAnswerMark(const GameState* game, uint8_t card, uint8_t player);

/**
 * @brief Serializes the current SAT knowledge base of a game to a DIMACS CNF file.
 *
 * The output includes SAT export comments and the DIMACS problem/clause lines,
 * written to the file at the given path.
 *
 * @param game Current game state.
 * @param path File path to write.
 * @return true if file open, serialization, and close all succeed; false otherwise.
 */
bool kb_serializeProblem(const GameState* game, const char* path);

//***************************************************************************//
//**************************       SAT PIERCE-THROUGH      ******************//
//***************************************************************************//

/**
 * @brief Pushes a temporary SAT checkpoint level for speculative reasoning.
 *
 * @param game Current game state.
 */
void kb_pushLevel(GameState* game);

/**
 * @brief Pops the current SAT checkpoint level.
 *
 * @param game Current game state.
 */
void kb_popLevel(GameState* game);

/**
 * @brief Checks satisfiability of the current knowledge base state.
 *
 * @param game Current game state.
 * @return true if satisfiable, false otherwise.
 */
bool kb_isSatisfiable(const GameState* game);

/**
 * @brief Checks satisfiability under additional temporary literal assumptions.
 *
 * @param game Current game state.
 * @param numAssumptions Number of assumption literals.
 * @param assumptions Array of assumption literals.
 * @return true if satisfiable, false otherwise.
 */
bool kb_isSatisfiableWithAssumptions(const GameState* game, uint32_t numAssumptions, const SAT_Literal* assumptions);
