/**
 * @file sat_util.c
 */


//***************************************************************************//
//************************** INCLUDES ***************************************//
//***************************************************************************//

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include "sat_util.h"

//***************************************************************************//
//**************************  PRIVATE DEFINES *******************************//
//***************************************************************************//

/** @brief Growth factor for dynamic arrays, used in sat_util_ensureCapacity(). */
#define SAT_UTIL_GROWTH_FACTOR 2u

//***************************************************************************//
//************************** PRIVATE TYPEDEFS *******************************//
//***************************************************************************//


//***************************************************************************//
//************************** PRIVATE VARIABLE DECLARATIONS ******************//
//***************************************************************************//


//***************************************************************************//
//************************** PRIVATE FUNCTION DECLARATIONS ******************//
//***************************************************************************//


//***************************************************************************//
//************************** PRIVATE FUNCTION DEFINITIONS *******************//
//***************************************************************************//


//***************************************************************************//
//************************** PUBLIC FUNCTION DEFINITIONS ********************//
//***************************************************************************//

void sat_util_ensureCapacity(void** array, size_t* capacity, size_t newCapacity, size_t elementSize)
{
    if(*capacity >= newCapacity)
    {
        return;
    }

    if(*capacity == 0u)
    {
        *capacity = 1u;
    }

    while(*capacity < newCapacity)
    {
        *capacity *= SAT_UTIL_GROWTH_FACTOR;
    }
    *array = realloc(*array, (*capacity) * elementSize);
}

uint32_t sat_util_getVar(SAT_Literal literal)
{
    return (literal > 0) ? (literal - 1u) : (-literal - 1u);
}

bool sat_util_isValidLiteral(const SAT_Problem* problem, SAT_Literal lit)
{
    if(lit == 0)
    {
        return false;
    }
    if(lit < -(int32_t)problem->numVars)
    {
        return false;
    }
    if(lit > (int32_t)problem->numVars)
    {
        return false;
    }
    if(sat_util_getVar(lit) >= CDCL_MAX_NUM_VARS)
    {
        return false;
    }
    return true;
}

void sat_util_addClause(SAT_Problem* problem, Clause* clause)
{
    sat_util_ensureCapacity((void**)&problem->clauses, &problem->capacity, problem->numClauses + 1u, sizeof(Clause*));
    problem->clauses[problem->numClauses++] = clause;
}
