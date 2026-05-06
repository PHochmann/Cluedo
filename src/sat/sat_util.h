/**
 * @file sat_util.h
 */


#pragma once

//***************************************************************************//
//************************** INCLUDES ***************************************//
//***************************************************************************//

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "cdcl.h"

//***************************************************************************//
//**************************  PUBLIC DEFINES ********************************//
//***************************************************************************//


//***************************************************************************//
//************************** PUBLIC TYPEDEFS ********************************//
//***************************************************************************//


//***************************************************************************//
//************************** PUBLIC VARIABLE DECLARATIONS *******************//
//***************************************************************************//


//***************************************************************************//
//************************** PUBLIC FUNCTION DECLARATIONS *******************//
//***************************************************************************//

/**
 * @brief Ensures that a dynamically growing array has at least the specified capacity, reallocating if necessary.
 *
 * @param array Pointer to the array pointer to grow.
 * @param capacity Pointer to the current capacity of the array.
 * @param newCapacity The desired minimum capacity.
 * @param elementSize Size of each element in the array.
 */
void sat_util_ensureCapacity(void** array, size_t* capacity, size_t newCapacity, size_t elementSize);

/**
 * @brief Gets the 0-based variable index for a literal.
 *
 * @param literal Literal (positive or negative).
 * @return 0-based variable index.
 */
uint32_t sat_util_getVar(SAT_Literal literal);

/**
 * @brief Checks whether a literal is within the valid range for the given problem.
 *
 * @param problem SAT problem.
 * @param lit Literal to validate.
 * @return true if the literal is valid, false otherwise.
 */
bool sat_util_isValidLiteral(const SAT_Problem* problem, SAT_Literal lit);

/**
 * @brief Appends a clause pointer to the SAT problem's clause array, growing it if necessary.
 *
 * @param problem SAT problem.
 * @param clause Clause to add.
 */
void sat_util_addClause(SAT_Problem* problem, Clause* clause);
