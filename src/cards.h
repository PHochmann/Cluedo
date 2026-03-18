/**
 * @file cards.h
 */


#pragma once

//***************************************************************************//
//************************** INCLUDES ***************************************//
//***************************************************************************//

#include <stdint.h>

//***************************************************************************//
//************************** PUBLIC TYPEDEFS ********************************//
//***************************************************************************//


//***************************************************************************//
//************************** PUBLIC VARIABLE DEFINITIONS ********************//
//***************************************************************************//


//***************************************************************************//
//************************** PUBLIC FUNCTION DECLARATIONS *******************//
//***************************************************************************//

/**
 * @brief Get person card name by person index.
 *
 * @param personIndex Person index.
 * @return Person name.
 */
const char* cards_getPersonName(uint8_t personIndex);

/**
 * @brief Get weapon card name by weapon index.
 *
 * @param weaponIndex Weapon index.
 * @return Weapon name.
 */
const char* cards_getWeaponName(uint8_t weaponIndex);

/**
 * @brief Get room card name by room index.
 *
 * @param roomIndex Room index.
 * @return Room name.
 */
const char* cards_getRoomName(uint8_t roomIndex);

/**
 * @brief Get card name by card index.
 *
 * @param cardIndex Card index in [0, NUM_CARDS).
 * @return Card name or "(nil)" for invalid index.
 */
const char* cards_getCardName(uint8_t cardIndex);
