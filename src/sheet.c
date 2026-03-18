/**
 * @file sheet.c
 */


//***************************************************************************//
//************************** INCLUDES ***************************************//
//***************************************************************************//

#define Red   "\033[38;5;196m"
#define Green "\033[38;5;40m"
#define RST   "\033[0m"

#include "cards.h"
#include "table.h"
#include "kb.h"
#include "sheet.h"

//***************************************************************************//
//************************** PRIVATE FUNCTION DECLARATIONS ******************//
//***************************************************************************//

static const char* getColorByAssignment(int8_t value);
static int8_t      getAssignmentOfCard(int8_t* assignment, int8_t card, int8_t player);
static const char* getCardStateString(int8_t* assignment, uint8_t card, uint8_t player);
static void        add_card_section(Table* tbl, const GameState* game, uint8_t count,
                                   uint8_t (*getCard)(uint8_t),
                                   const char* (*getName)(uint8_t));

//***************************************************************************//
//************************** PRIVATE FUNCTION DEFINITIONS *******************//
//***************************************************************************//

static int8_t getAssignmentOfCard(int8_t* assignment, int8_t card, int8_t player)
{
    return assignment[kb_getVar(card, player)];
}

static const char* getColorByAssignment(int8_t value)
{
    if(value == 1)
    {
        return Green;
    }
    if(value == -1)
    {
        return Red;
    }
    return "";
}

static const char* getCardStateString(int8_t* assignment, uint8_t card, uint8_t player)
{
    int8_t value = getAssignmentOfCard(assignment, card, player);
    if(value == 1)
    {
        return Green "Y" RST;
    }
    if(value == -1)
    {
        return Red "X" RST;
    }
    if(value == 0)
    {
        return " ";
    }
    return "!";
}

static void add_card_section(Table* tbl, const GameState* game, uint8_t count,
                            uint8_t (*getCard)(uint8_t),
                            const char* (*getName)(uint8_t))
{
    for(uint8_t i = 0; i < count; i++)
    {
        tbl_add_cell_fmt(tbl, " %s%s" RST " ",
            getColorByAssignment(getAssignmentOfCard(game->currAssignment, getCard(i), ENVELOPE_PLAYER_ID)),
            getName(i));
        for(uint8_t j = 0; j < game->numPlayers; j++)
        {
            tbl_add_cell_fmt(tbl, "%s", getCardStateString(game->currAssignment, getCard(i), j));
        }
        tbl_next_row(tbl);
    }
    tbl_set_hline(tbl, TBL_BORDER_SINGLE);
}

//***************************************************************************//
//*************************** PUBLIC FUNCTION DEFINITIONS *******************//
//***************************************************************************//

void sheet_print(const GameState* game)
{
    Table* tbl = tbl_get_new();
    tbl_add_empty_cell(tbl);
    TableHAlign alignments[MAX_NUM_PLAYERS + 1u] = { TBL_H_ALIGN_LEFT };
    for(uint8_t i = 1u; i < game->numPlayers + 1u; i++)
    {
        alignments[i] = TBL_H_ALIGN_CENTER;
    }
    tbl_set_default_alignments(tbl, game->numPlayers + 1, alignments, NULL);
    tbl_set_vline(tbl, 1, TBL_BORDER_SINGLE);
    for(uint8_t i = 0; i < game->numPlayers; i++)
    {
        tbl_add_cell_fmt(tbl, " %c ", game->playerNames[i][0]);
    }
    tbl_next_row(tbl);
    tbl_set_hline(tbl, TBL_BORDER_DOUBLE);
    add_card_section(tbl, game, NUM_PERSONS, kb_getPersonCard, cards_getPersonName);
    add_card_section(tbl, game, NUM_WEAPONS, kb_getWeaponCard, cards_getWeaponName);
    add_card_section(tbl, game, NUM_ROOMS,   kb_getRoomCard,   cards_getRoomName);
    tbl_make_boxed(tbl, TBL_BORDER_SINGLE);
    tbl_print(tbl);
    tbl_free(tbl);
}
