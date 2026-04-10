/**
 * @file cards.c
 */


//***************************************************************************//
//************************** INCLUDES ***************************************//
//***************************************************************************//

#include "cards.h"
#include "kb.h"

//***************************************************************************//
//**************************        PRIVATE DEFINES        ******************//
//***************************************************************************//

/** @brief Index offset for suspect cards within the card array. */
#define SUSPECT_CARD_OFFSET 0u

/** @brief Index offset for weapon cards within the card array. */
#define WEAPON_CARD_OFFSET (NUM_SUSPECTS)

/** @brief Index offset for room cards within the card array. */
#define ROOM_CARD_OFFSET (NUM_SUSPECTS + NUM_WEAPONS)

//***************************************************************************//
//************************** PRIVATE VARIABLE DECLARATIONS ******************//
//***************************************************************************//

static const char* suspects[] = {
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

const char* cards_getSuspectName(uint8_t suspectIndex)
{
    return suspects[suspectIndex];
}

uint8_t cards_getSuspectCard(uint8_t suspectIndex)
{
    return suspectIndex + SUSPECT_CARD_OFFSET;
}

const char* cards_getWeaponName(uint8_t weaponIndex)
{
    return weapons[weaponIndex];
}

uint8_t cards_getWeaponCard(uint8_t weaponIndex)
{
    return weaponIndex + WEAPON_CARD_OFFSET;
}

const char* cards_getRoomName(uint8_t roomIndex)
{
    return rooms[roomIndex];
}

uint8_t cards_getRoomCard(uint8_t roomIndex)
{
    return roomIndex + ROOM_CARD_OFFSET;
}

const char* cards_getCardName(uint8_t cardIndex)
{
    if(cardIndex < WEAPON_CARD_OFFSET)
    {
        return cards_getSuspectName(cardIndex - cards_getSuspectCard(0u));
    }
    if(cardIndex < ROOM_CARD_OFFSET)
    {
        return cards_getWeaponName(cardIndex - cards_getWeaponCard(0u));
    }
    if(cardIndex < (ROOM_CARD_OFFSET + NUM_ROOMS))
    {
        return cards_getRoomName(cardIndex - cards_getRoomCard(0u));
    }
    return "(nil)";
}
