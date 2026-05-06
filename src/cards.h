/**
 * @file cards.h
 */


#pragma once

//***************************************************************************//
//************************** INCLUDES ***************************************//
//***************************************************************************//

#include <stdint.h>

//***************************************************************************//
//************************** PUBLIC FUNCTION DECLARATIONS *******************//
//***************************************************************************//

/**
 * @brief Gets suspect card name by suspect index.
 *
 * @param suspectIndex Suspect index.
 * @return Suspect name.
 */
const char* cards_getSuspectName(uint8_t suspectIndex);

/**
 * @brief Returns the card index for a suspect card given its suspect index.
 *
 * @param suspectIndex Index of the suspect (0..NUM_SUSPECTS-1).
 * @return Card index of the suspect card.
 */
uint8_t cards_getSuspectCard(uint8_t suspectIndex);

/**
 * @brief Gets weapon card name by weapon index.
 *
 * @param weaponIndex Weapon index.
 * @return Weapon name.
 */
const char* cards_getWeaponName(uint8_t weaponIndex);

/**
 * @brief Returns the card index for a weapon card given its weapon index.
 *
 * @param weaponIndex Index of the weapon (0..NUM_WEAPONS-1).
 * @return Card index of the weapon card.
 */
uint8_t cards_getWeaponCard(uint8_t weaponIndex);

/**
 * @brief Gets room card name by room index.
 *
 * @param roomIndex Room index.
 * @return Room name.
 */
const char* cards_getRoomName(uint8_t roomIndex);

/**
 * @brief Returns the card index for a room card given its room index.
 *
 * @param roomIndex Index of the room (0..NUM_ROOMS-1).
 * @return Card index of the room card.
 */
uint8_t cards_getRoomCard(uint8_t roomIndex);

/**
 * @brief Gets card name by card index.
 *
 * @param cardIndex Card index in [0, NUM_CARDS).
 * @return Card name or "(nil)" for invalid index.
 */
const char* cards_getCardName(uint8_t cardIndex);
