/**
 * @file main.c
 */


//***************************************************************************//
//************************** INCLUDES ***************************************//
//***************************************************************************//

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>

#include "cards.h"
#include "kb.h"
#include "recommender.h"
#include "sheet.h"

#include <readline/readline.h>

//***************************************************************************//
//************************** PRIVATE FUNCTION DECLARATIONS ******************//
//***************************************************************************//

// String lookups
static const char* getPlayerName(const GameState* game, uint8_t playerIndex);

// Parsing
static char*  vreadlineasprintf(const char* promptfmt, va_list args);
static char*  readlineasprintf(const char* promptfmt, ...);
static int8_t vparse(int8_t (*parseFunc)(const GameState* game, const char* str), const GameState* game, const char* fmt, va_list args);
static int8_t parse(int8_t (*parseFunc)(const GameState* game, const char* str), const GameState* game, const char* fmt, ...);
// parseFuncs
static int8_t parsePerson(const GameState* game, const char* str);
static int8_t parsePlayer(const GameState* game, const char* str);
static int8_t parseKeyPlayer(const GameState* game, const char* str);
static int8_t parseWeapon(const GameState* game, const char* str);
static int8_t parseRoom(const GameState* game, const char* str);
static int8_t parseCard(const GameState* game, const char* str);
static int8_t parseYesNo(const GameState* game, const char* str);
static int8_t parseDiceRoll(const GameState* game, const char* str);

// Sheet
// Game logic
static void printConflict(const GameState* game, const KB_ConflictInfo* conflict);
static void playGame(GameState* game);
static void gameLoop(void);

#define MIN_PLAYERS 3u

//***************************************************************************//
//************************** PRIVATE FUNCTION DEFINITIONS *******************//
//***************************************************************************//

static const char* getPlayerName(const GameState* game, uint8_t playerIndex)
{
    return game->playerNames[playerIndex];
}

int strcmp2(const char* s1, const char* s2)
{
    while(*s1 && *s2)
    {
        int c1 = tolower((unsigned char)*s1);
        int c2 = tolower((unsigned char)*s2);

        if(c1 != c2)
            return c1 - c2;

        s1++;
        s2++;
    }

    // Handle different lengths
    return (unsigned char)tolower(*s1) -
           (unsigned char)tolower(*s2);
}

static char* vreadlineasprintf(const char* fmt, va_list args)
{
    char* prompt = NULL;
    char* line;
    if(vasprintf(&prompt, fmt, args) == -1)
    {
        return NULL;
    }
    line = readline(prompt);
    free(prompt);
    return line;
}

static char* readlineasprintf(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    char* result = vreadlineasprintf(fmt, args);
    va_end(args);
    return result;
}

static int8_t parsePerson(const GameState* game, const char* str)
{
    (void)game;
    for(uint8_t i = 0u; i < NUM_PERSONS; i++)
    {
        if(strcmp2(str, cards_getPersonName(i)) == 0)
        {
            return i;
        }
    }
    return -1;
}

static int8_t parsePlayer(const GameState* game, const char* str)
{
    for(uint8_t i = 1u; i < game->numPlayers; i++) // Always skip envelope - this is never an accepted input
    {
        if(strcmp2(str, getPlayerName(game, i)) == 0)
        {
            return i;
        }
    }
    return -1;
}

static int8_t parseKeyPlayer(const GameState* game, const char* str)
{
    if(str[0] == 'n')
    {
        return -2;
    }
    return parsePlayer(game, str);
}

static int8_t parseWeapon(const GameState* game, const char* str)
{
    (void)game;

    for(uint8_t i = 0u; i < NUM_WEAPONS; i++)
    {
        if(strcmp2(str, cards_getWeaponName(i)) == 0)
        {
            return i;
        }
    }
    return -1;
}

static int8_t parseRoom(const GameState* game, const char* str)
{
    (void)game;

    for(uint8_t i = 0u; i < NUM_ROOMS; i++)
    {
        if(strcmp2(str, cards_getRoomName(i)) == 0)
        {
            return i;
        }
    }
    return -1;
}

static int8_t parseCard(const GameState* game, const char* str)
{
    (void)game;

    int8_t result;
    result = parsePerson(game, str);
    if(result != -1)
    {
        return kb_getPersonCard(result);
    }
    result = parseWeapon(game, str);
    if(result != -1)
    {
        return kb_getWeaponCard(result);
    }
    result = parseRoom(game, str);
    if(result != -1)
    {
        return kb_getRoomCard(result);
    }
    return -1;
}

static int8_t parseYesNo(const GameState* game, const char* str)
{
    (void)game;
    if(str[0] == 'n')
    {
        return 0;
    }
    if(str[0] == 'y')
    {
        return 1;
    }
    return -1;
}

static int8_t parseDiceRoll(const GameState* game, const char* str)
{
    (void)game;
    int8_t value = atoi(str);
    if((value >= 1) && (value <= 3))
    {
        return value;
    }
    return -1;
}

static int8_t parseCardNumber(const GameState* game, const char* str)
{
    (void)game;
    int8_t value = atoi(str);
    if((value >= 3) && (value <= 5))
    {
        return value;
    }
    return -1;
}

static int8_t vparse(int8_t (*parseFunc)(const GameState* game, const char* str), const GameState* game, const char* fmt, va_list args)
{
    int8_t result = -1;
    while(result == -1)
    {
        va_list argsCpy;
        va_copy(argsCpy, args);
        char* input = vreadlineasprintf(fmt, argsCpy);
        va_end(argsCpy);
        if(input != NULL)
        {
            result = parseFunc(game, input);
            if(result == -1)
            {
                printf("Invalid input.\n");
            }
            free(input);
        }
    }
    return result;
}

static int8_t parse(int8_t (*parseFunc)(const GameState* game, const char* str), const GameState* game, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int8_t result = vparse(parseFunc, game, fmt, args);
    va_end(args);
    return result;
}

static void printConflict(const GameState* game, const KB_ConflictInfo* conflict)
{
    printf("At least one of these statements has to be true:\n");
    for(uint32_t i = 0u; i < conflict->numLiterals; i++)
    {
        SAT_Literal lit = conflict->literals[i];
        uint32_t    var = abs(lit - 1);
        if(var >= NUM_CARD_VARIABLES)
        {
            printf("- Cardinality constraint of player cards violated\n");
        }
        else
        {
            bool    sign   = (lit > 0);
            uint8_t card   = var % NUM_CARDS;
            uint8_t player = var / NUM_CARDS;
            printf("- %s %s %s\n", getPlayerName(game, player), (sign == false) ? "does not have" : "has", cards_getCardName(card));
        }
    }
    printf("This is not possible, due to information you entered and the rules of the game.\n");
}

static bool updateAssignment(GameState* game)
{
    KB_ConflictInfo conflict;
    if(kb_updateAssignment(game, &conflict) == false)
    {
        printConflict(game, &conflict);
        return true;
    }
    return false;
}

static void playGame(GameState* game)
{
    // Initialize local variables that don't need to be in the game state struct
    uint8_t playerCharacter = parse(parsePerson, game, "Your character?> ");
    int8_t  currentRoom     = -1;
    uint8_t keys[MAX_NUM_PLAYERS];
    bool    isEliminated[MAX_NUM_PLAYERS] = { false };
    for(uint8_t i = 0u; i < MAX_NUM_PLAYERS; i++)
    {
        keys[i] = 1u;
    }

    // Ask about starting cards
    int8_t startCards[NUM_CARDS];
    for(uint8_t i = 0u; i < game->numCards[game->player]; i++)
    {
        bool ok = false;
        while(ok == false)
        {
            startCards[i] = parse(parseCard, game, "Your card %d?> ", i + 1u);
            ok            = true;
            for(uint8_t j = 0u; j < i; j++)
            {
                if(startCards[j] == startCards[i])
                {
                    printf("Card already entered.\n");
                    ok = false;
                }
            }
        }
    }
    kb_addStartCardClauses(game, game->player, game->numCards[game->player], startCards);
    if(updateAssignment(game) == true)
        return;

    uint8_t currentPlayer = parse(parsePlayer, game, "Who starts?> ");
    sheet_print(game);

    while(true)
    {
        if(currentPlayer == game->player)
        {
            printf("== Your turn ==\n");
            if(currentRoom == -1)
            {
                currentRoom = parse(parseRoom, game, "Your current room?> ");
            }
            int8_t diceRoll = parse(parseDiceRoll, game, "Your dice roll?> ");

            bool reachableRooms[NUM_ROOMS] = { false };
            for(uint8_t i = 0u; i < (diceRoll * 2) + 1; i++)
            {
                int8_t room          = (currentRoom - diceRoll + i + NUM_ROOMS) % NUM_ROOMS;
                reachableRooms[room] = true;
            }

            uint8_t p;
            uint8_t w;
            uint8_t r;
            recommender_getGuess(game, &p, &w, &r, reachableRooms);
            printf("You should guess: %s with %s in the %s\n", cards_getPersonName(p), cards_getWeaponName(w), cards_getRoomName(r));
            currentRoom = r;

            for(uint8_t i = 1u; i < game->numPlayers; i++)
            {
                if(i == game->player)
                {
                    continue;
                }
                int8_t yn = parse(parseYesNo, game, "Answer of %s (y/n)?> ", getPlayerName(game, i));
                kb_addGuessAnswerClauses(game, p, w, r, i, yn);

                if(yn == 1)
                {
                    game->positiveAnswers[kb_getVar(kb_getPersonCard(p), i)] = true;
                    game->positiveAnswers[kb_getVar(kb_getWeaponCard(w), i)] = true;
                    game->positiveAnswers[kb_getVar(kb_getRoomCard(r), i)] = true;
                }

                if(updateAssignment(game) == true)
                {
                    return;
                }
            }
            while((keys[game->player] > 0u) && (recommender_isEnvelopeDecided(game, &p, &w, &r) == false))
            {
                uint8_t keyPlayer;
                uint8_t keyGuessCard;
                if(recommender_getKeyGuess(game, &keyPlayer, &keyGuessCard) == true)
                {
                    printf("Use your key: Ask %s about %s\n", getPlayerName(game, keyPlayer), cards_getCardName(keyGuessCard));
                    keys[game->player]--;
                    keys[keyPlayer]++;
                    int8_t yn = parse(parseYesNo, game, "Answer of %s (y/n)?> ", getPlayerName(game, keyPlayer));
                    kb_addKeyGuessClauses(game, keyPlayer, keyGuessCard, yn);
                    if(updateAssignment(game) == true)
                        return;
                }
                else
                {
                    break;
                }
            }
            if(recommender_isEnvelopeDecided(game, &p, &w, &r) == true)
            {
                sheet_print(game);
                printf("Envelope is known, make accusation: %s with %s in the %s\n", cards_getPersonName(p), cards_getWeaponName(w), cards_getRoomName(r));
                printf("== Game ended: You won ==\n");
                break;
            }
        }
        else
        {
            if(isEliminated[currentPlayer] == true)
            {
                currentPlayer = currentPlayer % (game->numPlayers - 1u) + 1u;
                continue;
            }

            printf("== Turn of %s ==\n", getPlayerName(game, currentPlayer));

            int8_t p = parse(parsePerson, game, "Person guess?> ");
            int8_t w = parse(parseWeapon, game, "Weapon guess?> ");
            int8_t r = parse(parseRoom, game, "Room guess?> ");

            if(p == playerCharacter)
            {
                currentRoom = r;
            }

            for(uint8_t i = 1u; i < game->numPlayers; i++)
            {
                if((i == currentPlayer) || (i == game->player))
                {
                    continue;
                }
                int8_t yn = parse(parseYesNo, game, "Answer of %s (y/n)?> ", getPlayerName(game, i));
                if(yn == 1)
                {
                    game->positiveAnswers[kb_getVar(kb_getPersonCard(p), i)] = true;
                    game->positiveAnswers[kb_getVar(kb_getWeaponCard(w), i)] = true;
                    game->positiveAnswers[kb_getVar(kb_getRoomCard(r), i)] = true;
                }
                kb_addGuessAnswerClauses(game, p, w, r, i, yn);
                if(updateAssignment(game) == true)
                {
                    return;
                }
            }

            while(keys[currentPlayer] > 0u)
            {
                int8_t keyPl = parse(parseKeyPlayer, game, "Is key used (player or n)?> ");
                if(keyPl == -2)
                {
                    break;
                }
                keys[currentPlayer]--;
                keys[keyPl]++;
            }

            int8_t accusationYN = parse(parseYesNo, game, "Is player making accusation (y/n)?> ");
            if(accusationYN == 1u)
            {
                int8_t correctYN = parse(parseYesNo, game, "Is accusation correct (y/n)?> ");
                if(correctYN == 1u)
                {
                    sheet_print(game);
                    printf("== Game ended: %s won. ==\n", getPlayerName(game, currentPlayer));
                    break;
                }
                else
                {
                    int8_t p = parse(parsePerson, game, "Person accusation?> ");
                    int8_t w = parse(parseWeapon, game, "Weapon accusation?> ");
                    int8_t r = parse(parseRoom, game, "Room accusation?> ");
                    kb_addFailedAccusationClauses(game, p, w, r);
                    if(updateAssignment(game) == true)
                    {
                        return;
                    }
                    isEliminated[currentPlayer] = true;
                    printf("%s is eliminated.\n", getPlayerName(game, currentPlayer));
                }
            }
        }
        currentPlayer = currentPlayer % (game->numPlayers - 1u) + 1u;
        sheet_print(game);
    }

    char* input = readline("Press enter for next round.");
    free(input);
}

static void gameLoop(void)
{
    printf("== Start of game ==\n");

    GameState game;
    kb_newGame(&game);

    // Ask about players and their names
    game.playerNames[0] = "Envelope";
    uint8_t i;
    for(i = 1u; i < MAX_NUM_PLAYERS; i++)
    {
        bool ok = false;
        while(ok == false)
        {
            game.playerNames[i] = readlineasprintf("Player at position %u%s?> ", i, (i < MIN_PLAYERS) ? "" : " (or x when no other player)");
            ok                  = true;

            if((game.playerNames[i][0] == 'x') || (game.playerNames[i][0] == 'X'))
            {
                if(i < MIN_PLAYERS)
                {
                    printf("At least %u players are required.\n", MIN_PLAYERS - 1u);
                    ok = false;
                }
                else
                {
                    free(game.playerNames[i]);
                    ok = false;
                    break;
                }
            }

            for(uint8_t j = 1u; j < i; j++)
            {
                if(strcmp2(game.playerNames[i], game.playerNames[j]) == 0)
                {
                    printf("Name already entered.\n");
                    ok = false;
                }
            }
        }

        if(ok == false) // X entered
        {
            break;
        }
    }

    // Build initial knowledge base
    game.numPlayers = i;

    if((game.numPlayers == 5u) || (game.numPlayers == 6u)) // Includes envelope, so 4 and 5 players
    {
        uint8_t totalCards = 0u;
        // Ask about card distribution for 4 and 5 players, as this is not even
        for(uint8_t i = 1u; i < game.numPlayers; i++)
        {
            game.numCards[i] = parse(parseCardNumber, &game, "Number of cards of %s?> ", getPlayerName(&game, i));
            totalCards += game.numCards[i];
        }
        if(totalCards != (NUM_CARDS - 3u))
        {
            printf("Not exactly %u cards assigned to players.\n", NUM_CARDS - 3u);
            kb_freeGame(&game);
            return;
        }
    }
    else
    {
        for(uint8_t i = 1u; i < game.numPlayers; i++)
        {
            game.numCards[i] = (NUM_CARDS - 3u) / (game.numPlayers - 1u);
        }
    }

    game.player = parse(parsePlayer, &game, "Your player?> ");
    kb_addRulesetClauses(&game);
    if(updateAssignment(&game) == true)
    {
        printf("Software defect - logical error in initial knowledge base\n");
        exit(1);
    }

    playGame(&game);
    kb_freeGame(&game);
}

//***************************************************************************//
//*************************** PUBLIC FUNCTION DEFINITIONS *******************//
//***************************************************************************//

int main()
{
    while(true)
    {
        gameLoop();
        printf("\n");
    }
}
