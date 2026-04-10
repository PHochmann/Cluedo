/**
 * @file main.c
 */


//***************************************************************************//
//************************** INCLUDES ***************************************//
//***************************************************************************//

#include <stdio.h>
#include <stdlib.h>
#include <readline/readline.h>

#include "cards.h"
#include "kb.h"
#include "recommender.h"
#include "sheet.h"
#include "parsing.h"


//***************************************************************************//
//**************************        PRIVATE DEFINES        ******************//
//***************************************************************************//

#define MIN_PLAYERS 3u

//***************************************************************************//
//**************************        PRIVATE TYPEDEFS       ******************//
//***************************************************************************//

/**
 * @brief Tracks game state that remains private to the main game loop (outside GameState).
 * 
 * @var GamePrivateState::keys            Number of keys for each player.
 * @var GamePrivateState::isEliminated    Whether each player has been eliminated by a failed accusation.
 * @var GamePrivateState::currentRoom     Current room of the player. -1 if unknown (game start).
 * @var GamePrivateState::playerCharacter Character index of the player (0..NUM_SUSPECTS-1).
 */
typedef struct
{
    uint8_t keys[MAX_NUM_PLAYERS];
    bool    isEliminated[MAX_NUM_PLAYERS];
    int8_t  currentRoom;
    uint8_t playerCharacter;
} GamePrivateState;

/**
 * @brief Describes the result of handling one game turn.
 */
typedef enum
{
    GAME_TURN_CONTINUE,
    GAME_TURN_END,
    GAME_TURN_INFEASIBLE
} GameTurnOutcome;

//***************************************************************************//
//************************** PRIVATE FUNCTION DECLARATIONS ******************//
//***************************************************************************//

// String lookups
/**
 * @brief Todo
 * @param settings Todo
 * @param playerIndex Todo
 * @return Todo
 */
static const char* getPlayerName(const GameSettings* settings, uint8_t playerIndex);

// Game state helpers
/**
 * @brief Todo
 * @param state Todo
 */
static void initializeRoundState(GamePrivateState* state);
/**
 * @brief Todo
 * @param currentRoom Todo
 * @param diceRoll Todo
 * @param reachableRooms Todo
 */
static void calculateReachableRooms(int8_t currentRoom, int8_t diceRoll, bool* reachableRooms);
/**
 * @brief Todo
 * @param settings Todo
 * @param player Todo
 * @param startCards Todo
 */
static void getStartingCards(const GameSettings* settings, uint8_t player, int8_t* startCards);
/**
 * @brief Todo
 * @param settings Todo
 * @param game Todo
 * @param guessingPlayer Todo
 * @param suspect Todo
 * @param weapon Todo
 * @param room Todo
 * @return Todo
 */
static GameTurnOutcome processGuessResponses(const GameSettings* settings, GameState* game, uint8_t guessingPlayer, uint8_t suspect, uint8_t weapon, uint8_t room);

// Setup helpers
/**
 * @brief Todo
 * @param settings Todo
 */
static void setupPlayers(GameSettings* settings);
/**
 * @brief Todo
 * @param settings Todo
 */
static void setupCardDistribution(GameSettings* settings);
/**
 * @brief Todo
 * @param settings Todo
 */
static void freeSettings(GameSettings* settings);

// Turn handlers
/**
 * @brief Todo
 * @param settings Todo
 * @param game Todo
 * @param roundState Todo
 * @return Todo
 */
static GameTurnOutcome playerTurn(const GameSettings* settings, GameState* game, GamePrivateState* roundState);
/**
 * @brief Todo
 * @param settings Todo
 * @param game Todo
 * @param player Todo
 * @param roundState Todo
 * @return Todo
 */
static GameTurnOutcome otherPlayerTurn(const GameSettings* settings, GameState* game, uint8_t player, GamePrivateState* roundState);

// Game logic
/**
 * @brief Todo
 * @param settings Todo
 * @param game Todo
 * @param privateState Todo
 */
static void gameLoop(const GameSettings* settings, GameState* game, GamePrivateState* privateState);
/**
 * @brief Todo
 */
static void playGame(void);

//***************************************************************************//
//************************** PRIVATE FUNCTION DEFINITIONS *******************//
//***************************************************************************//

static const char* getPlayerName(const GameSettings* settings, uint8_t playerIndex)
{
    return settings->playerNames[playerIndex];
}

//***************************************************************************//
//************************** GAME STATE HELPERS *****************************//
//***************************************************************************//

static void initializeRoundState(GamePrivateState* state)
{
    for(uint8_t i = 0u; i < MAX_NUM_PLAYERS; i++)
    {
        state->keys[i]         = 1u;
        state->isEliminated[i] = false;
    }
    state->currentRoom = -1;
}

static void calculateReachableRooms(int8_t currentRoom, int8_t diceRoll, bool* reachableRooms)
{
    for(uint8_t i = 0u; i < (diceRoll * 2) + 1; i++)
    {
        int8_t room          = (currentRoom - diceRoll + i + NUM_ROOMS) % NUM_ROOMS;
        reachableRooms[room] = true;
    }
}

static void getStartingCards(const GameSettings* settings, uint8_t player, int8_t* startCards)
{
    for(uint8_t i = 0u; i < settings->numCards[player]; i++)
    {
        bool ok = false;
        while(ok == false)
        {
            startCards[i] = parsing_getCard(settings, "Your card %d?> ", i + 1u);
            ok            = true;

            bool duplicateCard = false;
            for(uint8_t j = 0u; j < i; j++)
            {
                if(startCards[j] == startCards[i])
                {
                    duplicateCard = true;
                    break;
                }
            }
            if(duplicateCard == true)
            {
                printf("Card already entered.\n");
                ok = false;
            }
        }
    }
}

static GameTurnOutcome processGuessResponses(const GameSettings* settings, GameState* game, uint8_t guessingPlayer, uint8_t suspect, uint8_t weapon, uint8_t room)
{
    for(uint8_t i = 1u; i < settings->numPlayers; i++)
    {
        if((i == guessingPlayer) || (i == settings->humanPlayer))
        {
            continue;
        }

        SAT_Assignment answerFromKb = SAT_UNKNOWN;
        SAT_Assignment suspectValue = kb_getAssignmentValue(game, cards_getSuspectCard(suspect), i);
        SAT_Assignment weaponValue  = kb_getAssignmentValue(game, cards_getWeaponCard(weapon), i);
        SAT_Assignment roomValue    = kb_getAssignmentValue(game, cards_getRoomCard(room), i);
        if((suspectValue == SAT_TRUE) || (weaponValue == SAT_TRUE) || (roomValue == SAT_TRUE))
        {
            answerFromKb = SAT_TRUE;
        }
        else if((suspectValue == SAT_FALSE) && (weaponValue == SAT_FALSE) && (roomValue == SAT_FALSE))
        {
            answerFromKb = SAT_FALSE;
        }

        bool response = parsing_getYesNo("Answer of %s %s (y/n)?> ", getPlayerName(settings, i), (answerFromKb == SAT_TRUE) ? "(known to be yes)" : (answerFromKb == SAT_FALSE) ? "(known to be no)"
                                                                                                                                                                                : "");
        kb_addGuessAnswerClauses(game, suspect, weapon, room, i, response, true);
        if(kb_updateAssignment(game) == false)
        {
            return GAME_TURN_INFEASIBLE;
        }
    }
    return GAME_TURN_CONTINUE;
}

//***************************************************************************//
//************************** SETUP HELPERS *********************************//
//***************************************************************************//

static void setupPlayers(GameSettings* settings)
{
    for(uint8_t pl = 1u; pl < MAX_NUM_PLAYERS; pl++)
    {
        bool ok = false;
        while(ok == false)
        {
            settings->playerNames[pl] = parsing_readLine("Player at position %u%s?> ", pl, (pl < MIN_PLAYERS) ? "" : " (or x when no other player)");
            ok                        = true;

            if((settings->playerNames[pl][0] == 'x') || (settings->playerNames[pl][0] == 'X'))
            {
                if(pl < MIN_PLAYERS)
                {
                    printf("At least %u players are required.\n", MIN_PLAYERS - 1u);
                    free(settings->playerNames[pl]);
                    ok = false;
                    continue;
                }
                else
                {
                    free(settings->playerNames[pl]);
                    settings->numPlayers = pl;
                    return;
                }
            }

            bool duplicateName = false;
            for(uint8_t i = 1u; i < pl; i++)
            {
                if(parsing_strcmp2(settings->playerNames[i], settings->playerNames[pl]) == 0)
                {
                    duplicateName = true;
                    break;
                }
            }
            if(duplicateName == true)
            {
                printf("Name already entered.\n");
                free(settings->playerNames[pl]);
                ok = false;
            }
        }
    }
    settings->numPlayers = MAX_NUM_PLAYERS;
}

static void setupCardDistribution(GameSettings* settings)
{
    if((settings->numPlayers == 5u) || (settings->numPlayers == 6u))
    {
        while(true)
        {
            uint8_t totalCards = 0u;
            for(uint8_t pl = 1u; pl < settings->numPlayers; pl++)
            {
                settings->numCards[pl] = parsing_getCardNumber(settings, "Number of cards of %s?> ", settings->playerNames[pl]);
                totalCards += settings->numCards[pl];
            }
            if(totalCards != (NUM_CARDS - 3u))
            {
                printf("Not exactly %u cards assigned to players.\n", NUM_CARDS - 3u);
                continue;
            }
            break;
        }
    }
    else
    {
        for(uint8_t pl = 1u; pl < settings->numPlayers; pl++)
        {
            settings->numCards[pl] = (NUM_CARDS - 3u) / (settings->numPlayers - 1u);
        }
    }
}

static void freeSettings(GameSettings* settings)
{
    for(uint8_t i = 1u; i < settings->numPlayers; i++)
    {
        free(settings->playerNames[i]);
    }
}

//***************************************************************************//
//************************** TURN HANDLERS *********************************//
//***************************************************************************//

static GameTurnOutcome playerTurn(const GameSettings* settings, GameState* game, GamePrivateState* roundState)
{
    printf("== Your turn ==\n");

    if(roundState->currentRoom == -1)
    {
        roundState->currentRoom = parsing_getRoom(settings, "Your current room?> ");
    }

    int8_t diceRoll                  = parsing_getDiceRoll("Your dice roll?> ");
    bool   reachableRooms[NUM_ROOMS] = { false };
    calculateReachableRooms(roundState->currentRoom, diceRoll, reachableRooms);

    uint8_t suspect;
    uint8_t weapon;
    uint8_t room;

    recommender_getGuess(settings, game, reachableRooms, &suspect, &weapon, &room);
    printf("You should guess: %s with %s in the %s\n", cards_getSuspectName(suspect), cards_getWeaponName(weapon), cards_getRoomName(room));
    roundState->currentRoom = room;

    if(processGuessResponses(settings, game, settings->humanPlayer, suspect, weapon, room) == GAME_TURN_INFEASIBLE)
    {
        return GAME_TURN_INFEASIBLE;
    }

    while((roundState->keys[settings->humanPlayer] > 0u) && (recommender_isEnvelopeDecided(game, &suspect, &weapon, &room) == false))
    {
        uint8_t keyPlayer;
        uint8_t keyGuessCard;
        if(recommender_getKeyGuess(game, settings, &keyPlayer, &keyGuessCard) == true)
        {
            printf("Use your key: Ask %s about %s\n", getPlayerName(settings, keyPlayer), cards_getCardName(keyGuessCard));
            roundState->keys[settings->humanPlayer]--;
            roundState->keys[keyPlayer]++;

            bool response = parsing_getYesNo("Answer of %s (y/n)?> ", getPlayerName(settings, keyPlayer));
            kb_addKeyGuessClauses(game, keyPlayer, keyGuessCard, response);

            if(kb_updateAssignment(game) == false)
            {
                return GAME_TURN_INFEASIBLE;
            }
        }
        else
        {
            break;
        }
    }

    if(recommender_isEnvelopeDecided(game, &suspect, &weapon, &room) == true)
    {
        sheet_print(game, settings);
        printf("Envelope is known, make accusation: %s with %s in the %s\n", cards_getSuspectName(suspect), cards_getWeaponName(weapon), cards_getRoomName(room));
        printf("== Game ended: You won ==\n");
        return GAME_TURN_END;
    }

    return GAME_TURN_CONTINUE;
}

static GameTurnOutcome otherPlayerTurn(const GameSettings* settings, GameState* game, uint8_t player, GamePrivateState* roundState)
{
    if(roundState->isEliminated[player])
    {
        return GAME_TURN_CONTINUE; // Skip turn of eliminated player
    }

    printf("== Turn of %s ==\n", getPlayerName(settings, player));

    uint8_t suspect = parsing_getSuspect(settings, "Suspect guess?> ");
    uint8_t weapon  = parsing_getWeapon(settings, "Weapon guess?> ");
    uint8_t room    = parsing_getRoom(settings, "Room guess?> ");

    if(suspect == roundState->playerCharacter)
    {
        roundState->currentRoom = room;
    }

    if(processGuessResponses(settings, game, player, suspect, weapon, room) == GAME_TURN_INFEASIBLE)
    {
        return GAME_TURN_INFEASIBLE;
    }

    while(roundState->keys[player] > 0u)
    {
        int8_t keyPlayer = parsing_getKeyPlayer(settings, "Is key used (player or n)?> ");
        if(keyPlayer == PARSING_NO_KEY_PLAYER)
        {
            break; // "n" entered
        }
        roundState->keys[player]--;
        roundState->keys[keyPlayer]++;
    }

    bool makesAccusation = parsing_getYesNo("Is player making accusation (y/n)?> ");
    if(makesAccusation == true)
    {
        bool correct = parsing_getYesNo("Is accusation correct (y/n)?> ");
        if(correct == true)
        {
            sheet_print(game, settings);
            printf("== Game ended: %s won. ==\n", getPlayerName(settings, player));
            return GAME_TURN_END;
        }
        else
        {
            uint8_t accSuspect = parsing_getSuspect(settings, "Suspect accusation?> ");
            uint8_t accWeapon  = parsing_getWeapon(settings, "Weapon accusation?> ");
            uint8_t accRoom    = parsing_getRoom(settings, "Room accusation?> ");

            kb_addFailedAccusationClauses(game, accSuspect, accWeapon, accRoom);
            if(kb_updateAssignment(game) == false)
            {
                return GAME_TURN_INFEASIBLE;
            }
            roundState->isEliminated[player] = true;
            printf("%s is eliminated.\n", getPlayerName(settings, player));
        }
    }

    return GAME_TURN_CONTINUE;
}

static void gameLoop(const GameSettings* settings, GameState* game, GamePrivateState* roundState)
{
    uint8_t currentPlayer = parsing_getPlayer(settings, "Who starts?> ");
    sheet_print(game, settings);

    while(true)
    {
        GameTurnOutcome turnOutcome;
        if(currentPlayer == settings->humanPlayer)
        {
            turnOutcome = playerTurn(settings, game, roundState);
        }
        else
        {
            turnOutcome = otherPlayerTurn(settings, game, currentPlayer, roundState);
        }

        if(turnOutcome == GAME_TURN_INFEASIBLE)
        {
            printf("This is not possible, due to information you entered and the rules of the game.\n");
            break;
        }

        if(turnOutcome == GAME_TURN_END)
        {
            break; // Game ended
        }

        sheet_print(game, settings);

        currentPlayer = currentPlayer % (settings->numPlayers - 1u) + 1u;
    }
}

static void playGame(void)
{
    printf("== Start of game ==\n");

    // Settings setup
    GameSettings settings = { 0 };
    setupPlayers(&settings);
    setupCardDistribution(&settings);

    // Game state setup
    GameState* game = kb_newGame(&settings);

    // Private state setup
    GamePrivateState roundState;
    initializeRoundState(&roundState);
    settings.humanPlayer       = parsing_getPlayer(&settings, "Which player are you?> ");
    roundState.playerCharacter = parsing_getSuspect(&settings, "Which character are you?> ");

    // Add starting card clauses
    int8_t startCards[NUM_CARDS];
    getStartingCards(&settings, settings.humanPlayer, startCards);
    kb_addStartCardClauses(game, &settings, settings.humanPlayer, startCards);
    kb_updateAssignment(game);

    gameLoop(&settings, game, &roundState);
    kb_freeGame(game);
    freeSettings(&settings);

    char* input = readline("Press enter for next round.");
    free(input);
}

//***************************************************************************//
//*************************** PUBLIC FUNCTION DEFINITIONS *******************//
//***************************************************************************//

/**
 * @brief Todo
 * @return Todo
 */
int main()
{
    while(true)
    {
        playGame();
        printf("\n");
    }
}
