/**
 * @file sheet.c
 */


//***************************************************************************//
//************************** INCLUDES ***************************************//
//***************************************************************************//

#include "cards.h"
#include "table.h"
#include "kb.h"
#include "sheet.h"


//***************************************************************************//
//**************************        PRIVATE DEFINES        ******************//
//***************************************************************************//

#define Red    "\033[38;5;196m"
#define Green  "\033[38;5;40m"
#define Yellow "\033[38;5;226m"
#define RST    "\033[0m"

//***************************************************************************//
//************************** PRIVATE FUNCTION DECLARATIONS ******************//
//***************************************************************************//

/**
 * @brief Returns the ANSI color escape code for a SAT assignment.
 * @param value Assignment value to map to a display color.
 * @return Green for true, red for false, or an empty string otherwise.
 */
static const char* getColorByAssignment(SAT_Assignment value);

/**
 * @brief Builds the display marker for one card/player table cell.
 * @param game Current game state containing assignments and answer marks.
 * @param card Card identifier to display.
 * @param player Player identifier to display.
 * @return Colored "Y", "X", "?" or a blank space
 */
static const char* getCardStateString(const GameState* game, uint8_t card, uint8_t player);

/**
 * @brief Appends one card category section to the deduction sheet table.
 * @param tbl Table that receives the generated rows.
 * @param game Current game state used to resolve known card ownership.
 * @param settings Game settings containing player count.
 * @param count Number of cards in the category.
 * @param getCard Callback that maps section index to card identifier.
 * @param getName Callback that maps section index to display name.
 */
static void add_card_section(Table* tbl, const GameState* game, const GameSettings* settings, uint8_t count, uint8_t (*getCard)(uint8_t), const char* (*getName)(uint8_t));

//***************************************************************************//
//************************** PRIVATE FUNCTION DEFINITIONS *******************//
//***************************************************************************//

static const char* getColorByAssignment(SAT_Assignment value)
{
    if(value == SAT_TRUE)
    {
        return Green;
    }
    if(value == SAT_FALSE)
    {
        return Red;
    }
    return "";
}

static const char* getCardStateString(const GameState* game, uint8_t card, uint8_t player)
{
    SAT_Assignment value = kb_getAssignmentValue(game, card, player);
    if(value == SAT_TRUE)
    {
        return Green "Y" RST;
    }
    if(value == SAT_FALSE)
    {
        return Red "X" RST;
    }
    if(kb_getPositiveAnswerMark(game, card, player) == true)
    {
        return Yellow "?" RST;
    }
    return " ";
}

static void add_card_section(Table* tbl, const GameState* game, const GameSettings* settings, uint8_t count, uint8_t (*getCard)(uint8_t), const char* (*getName)(uint8_t))
{
    for(uint8_t i = 0; i < count; i++)
    {
        tbl_add_cell_fmt(tbl, " %s%s" RST " ", getColorByAssignment(kb_getAssignmentValue(game, getCard(i), ENVELOPE_PLAYER_ID)), getName(i));
        for(uint8_t j = 1u; j < settings->numPlayers; j++)
        {
            tbl_add_cell_fmt(tbl, "%s", getCardStateString(game, getCard(i), j));
        }
        tbl_next_row(tbl);
    }
    tbl_set_hline(tbl, TBL_BORDER_SINGLE);
}

//***************************************************************************//
//*************************** PUBLIC FUNCTION DEFINITIONS *******************//
//***************************************************************************//

void sheet_print(const GameState* game, const GameSettings* settings)
{
    Table* tbl = tbl_get_new();
    tbl_add_empty_cell(tbl);
    TableHAlign alignments[MAX_NUM_PLAYERS + 1u] = { TBL_H_ALIGN_LEFT };
    for(uint8_t i = 1u; i < settings->numPlayers; i++)
    {
        alignments[i] = TBL_H_ALIGN_CENTER;
    }
    tbl_set_default_alignments(tbl, settings->numPlayers, alignments, NULL);
    tbl_set_vline(tbl, 1, TBL_BORDER_SINGLE);
    for(uint8_t i = 1u; i < settings->numPlayers; i++)
    {
        tbl_add_cell_fmt(tbl, " %c ", settings->playerNames[i][0]);
    }
    tbl_next_row(tbl);
    tbl_set_hline(tbl, TBL_BORDER_DOUBLE);
    add_card_section(tbl, game, settings, NUM_SUSPECTS, cards_getSuspectCard, cards_getSuspectName);
    add_card_section(tbl, game, settings, NUM_WEAPONS, cards_getWeaponCard, cards_getWeaponName);
    add_card_section(tbl, game, settings, NUM_ROOMS, cards_getRoomCard, cards_getRoomName);
    tbl_make_boxed(tbl, TBL_BORDER_SINGLE);
    tbl_print(tbl);
    tbl_free(tbl);
}
