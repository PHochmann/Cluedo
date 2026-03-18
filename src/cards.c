/**
 * @file cards.c
 */


//***************************************************************************//
//************************** INCLUDES ***************************************//
//***************************************************************************//

#include "cards.h"
#include "kb.h"

//***************************************************************************//
//************************** PRIVATE VARIABLE DECLARATIONS ******************//
//***************************************************************************//

static const char* persons[] = {
    "Mustard",
    "Plum",
    "Green",
    "Peacock",
    "Scarlett",
    "White"
};

static const char* weapons[] = {
    "Candlestick",
    "Dagger",
    "Pipe",
    "Revolver",
    "Rope",
    "Wrench"
};

static const char* rooms[] = {
    "Kitchen",
    "Ballroom",
    "Conservatory",
    "Billiard Room",
    "Library",
    "Study",
    "Hall",
    "Lounge",
    "Dining Room"
};

//***************************************************************************//
//*************************** PUBLIC FUNCTION DEFINITIONS *******************//
//***************************************************************************//

const char* cards_getPersonName(uint8_t personIndex)
{
    return persons[personIndex];
}

const char* cards_getWeaponName(uint8_t weaponIndex)
{
    return weapons[weaponIndex];
}

const char* cards_getRoomName(uint8_t roomIndex)
{
    return rooms[roomIndex];
}

const char* cards_getCardName(uint8_t cardIndex)
{
    if(cardIndex < WEAPON_CARD_OFFSET)
    {
        return cards_getPersonName(cardIndex);
    }
    if(cardIndex < ROOM_CARD_OFFSET)
    {
        return cards_getWeaponName(cardIndex - WEAPON_CARD_OFFSET);
    }
    if(cardIndex < (ROOM_CARD_OFFSET + NUM_ROOMS))
    {
        return cards_getRoomName(cardIndex - ROOM_CARD_OFFSET);
    }
    return "(nil)";
}
