/**
 * @file cdcl.h
 */


#pragma once

//***************************************************************************//
//************************** INCLUDES ***************************************//
//***************************************************************************//

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "sat.h"

//***************************************************************************//
//**************************  PUBLIC DEFINES ********************************//
//***************************************************************************//

/** @brief Maximum number of variables supported. */
#define CDCL_MAX_NUM_VARS 10000u

/** @brief Maximum number of nested checkpoint (push/pop) levels supported. */
#define CDCL_MAX_CHECKPOINT_LEVELS 8u

//***************************************************************************//
//************************** PUBLIC TYPEDEFS ********************************//
//***************************************************************************//

/**
 * @brief A clause in the SAT problem: a disjunction of literals stored as a flexible array.
 *
 * @var Clause::numLiterals Number of literals in the clause.
 * @var Clause::literals Flexible array of literals forming the clause.
 */
typedef struct
{
    uint32_t    numLiterals;
    SAT_Literal literals[];
} Clause;

/**
 * @brief Holds all clauses and checkpoint-level bookmarks for a SAT problem instance.
 *
 * @var SAT_Problem::numVars Number of Boolean variables.
 * @var SAT_Problem::numClauses Current number of clauses.
 * @var SAT_Problem::capacity Allocated capacity of the clause array.
 * @var SAT_Problem::clauses Heap-allocated array of pointers to clauses.
 * @var SAT_Problem::numLevels Number of active checkpoint levels.
 * @var SAT_Problem::levels Clause-count snapshot for each checkpoint level.
 */
struct SAT_Problem
{
    size_t   numVars;
    size_t   numClauses;
    size_t   capacity;
    Clause** clauses;
    size_t   numLevels;
    size_t   levels[CDCL_MAX_CHECKPOINT_LEVELS];
};

//***************************************************************************//
//************************** PUBLIC VARIABLE DECLARATIONS *******************//
//***************************************************************************//


//***************************************************************************//
//************************** PUBLIC FUNCTION DECLARATIONS *******************//
//***************************************************************************//

/**
 * @brief Checks satisfiability with additional temporary literal assumptions using CDCL.
 *
 * @param problem SAT problem.
 * @param numAssumptions Number of assumption literals.
 * @param assumptions Array of assumption literals.
 * @param outResult If non-NULL, stores a heap-allocated SAT_Result* for both SAT and UNSAT outcomes.
 *                  Caller must free it with sat_freeResult().
 * @return true if satisfiable, false otherwise.
 */
bool cdcl_isSatisfiableWithAssumptions(
    SAT_Problem*       problem,
    uint32_t           numAssumptions,
    const SAT_Literal* assumptions,
    const SAT_Result** outResult);
