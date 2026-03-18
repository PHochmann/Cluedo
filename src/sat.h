/**
 * @file sat.h
 */


//***************************************************************************//
//************************** INCLUDES ***************************************//
//***************************************************************************//

#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

//***************************************************************************//
//**************************  PUBLIC TYPEDEFS *******************************//
//***************************************************************************//

typedef int32_t            SAT_Literal;
typedef struct SAT_Problem SAT_Problem;

//***************************************************************************//
//**************************  PUBLIC VARIABLE DECLARATIONS ******************//
//***************************************************************************//

/**
 * @brief Todo
 * 
 * @param problem Todo
 */
void sat_freeProblem(SAT_Problem* problem);

/**
 * @brief Todo
 * 
 * @param numVars Todo
 * @return Todo
 */
SAT_Problem* sat_initProblem(uint16_t numVars);

/**
 * @brief Todo
 * 
 * @param problem Todo
 * @param numLiterals Todo
 * @param literals Todo
 */
void sat_vAddClause(SAT_Problem* problem, uint32_t numLiterals, const SAT_Literal* literals);

/**
 * @brief Todo
 * 
 * @param problem Todo
 * @param numLiterals Todo
 */
void sat_addClause(SAT_Problem* problem, uint32_t numLiterals, ...);

/**
 * @brief Todo
 * 
 * @param problem Todo
 * @param literal Todo
 */
void sat_addUnitClause(SAT_Problem* problem, SAT_Literal literal);

/**
 * @brief Todo
 * 
 * @param problem Todo
 */
bool sat_pushLevel(SAT_Problem* problem);

/**
 * @brief Todo
 * 
 * @param problem Todo
 */
bool sat_popLevel(SAT_Problem* problem);

/**
 * @brief Todo
 * 
 * @param problem Todo
 * @param solution Todo
 * @param outNumLiterals Todo
 * @param outConflictClause Todo
 * @return Todo
 */
bool sat_isSatisfiable(SAT_Problem* problem, bool** solution, uint32_t* outNumLiterals, const SAT_Literal** outConflictClause);

/**
 * @brief Todo
 * 
 * @param problem Todo
 * @param numAssumptions Todo
 * @param assumptions Todo
 * @param solution Todo
 * @param outNumLiterals Todo
 * @param outConflictClause Todo
 * @return Todo
 */
bool sat_isSatisfiableWithAssumptions(
    SAT_Problem*        problem,
    uint32_t            numAssumptions,
    const SAT_Literal*  assumptions,
    bool**              solution,
    uint32_t*           outNumLiterals,
    const SAT_Literal** outConflictClause);

/**
 * @brief Todo
 * 
 * @param problem Todo
 * @param solution Todo
 * @param varIndex Todo
 * @return Todo
 */
int8_t sat_getBackboneValue(SAT_Problem* problem, bool* solution, uint32_t varIndex);

/**
 * @brief Todo
 * 
 * @param problem Todo
 * @return Todo
 */
size_t sat_getNumClauses(const SAT_Problem* problem);

/**
 * @brief Todo
 * 
 * @param problem Todo
 * @return Todo
 */
uint32_t sat_getNumVariables(const SAT_Problem* problem);
