/**
 * @file sat.h
 */


#pragma once

//***************************************************************************//
//************************** INCLUDES ***************************************//
//***************************************************************************//

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
 * @brief Frees all memory associated with a SAT problem.
 * 
 * @param problem The SAT problem to free.
 */
void sat_freeProblem(SAT_Problem* problem);

/**
 * @brief Allocates and initializes a new SAT problem with the given number of variables.
 * 
 * @param numVars Number of Boolean variables.
 * @return Pointer to the newly allocated SAT problem.
 */
SAT_Problem* sat_initProblem(uint16_t numVars);

/**
 * @brief Adds a clause (disjunction of literals) to the SAT problem using a pointer to a literal array.
 * 
 * @param problem The SAT problem.
 * @param numLiterals Number of literals in the clause.
 * @param literals Array of literals forming the clause.
 */
void sat_vAddClause(SAT_Problem* problem, uint32_t numLiterals, const SAT_Literal* literals);

/**
 * @brief Adds a clause to the SAT problem using variadic literal arguments.
 * 
 * @param problem The SAT problem.
 * @param numLiterals Number of literal arguments that follow.
 */
void sat_addClause(SAT_Problem* problem, uint32_t numLiterals, ...);

/**
 * @brief Adds a unit clause (single literal assertion) to the SAT problem.
 * 
 * @param problem The SAT problem.
 * @param literal The literal to assert.
 */
void sat_addUnitClause(SAT_Problem* problem, SAT_Literal literal);

/**
 * @brief Pushes a new decision level onto the solver's assumption stack.
 * 
 * @param problem The SAT problem.
 */
bool sat_pushLevel(SAT_Problem* problem);

/**
 * @brief Pops the top decision level from the assumption stack, undoing assumptions made at that level.
 * 
 * @param problem The SAT problem.
 */
bool sat_popLevel(SAT_Problem* problem);

/**
 * @brief Checks satisfiability of the current problem under all pushed assumptions.
 * 
 * @param problem The SAT problem.
 * @param solution If non-NULL and satisfiable, receives a heap-allocated satisfying assignment.
 * @param outNumLiterals If non-NULL and unsatisfiable, receives the number of conflict clause literals.
 * @param outConflictClause If non-NULL and unsatisfiable, receives a pointer to the conflict clause.
 * @return true if satisfiable, false otherwise.
 */
bool sat_isSatisfiable(SAT_Problem* problem, bool** solution, uint32_t* outNumLiterals, const SAT_Literal** outConflictClause);

/**
 * @brief Checks satisfiability with an additional set of assumptions without modifying the assumption stack.
 * 
 * @param problem The SAT problem.
 * @param numAssumptions Number of assumption literals.
 * @param assumptions Array of assumption literals.
 * @param solution If non-NULL and satisfiable, receives a heap-allocated satisfying assignment.
 * @param outNumLiterals If non-NULL and unsatisfiable, receives the number of conflict clause literals.
 * @param outConflictClause If non-NULL and unsatisfiable, receives a pointer to the conflict clause.
 * @return true if satisfiable, false otherwise.
 */
bool sat_isSatisfiableWithAssumptions(
    SAT_Problem*        problem,
    uint32_t            numAssumptions,
    const SAT_Literal*  assumptions,
    bool**              solution,
    uint32_t*           outNumLiterals,
    const SAT_Literal** outConflictClause);

/**
 * @brief Computes the backbone value of a variable: forced true (1), forced false (-1), or undetermined (0).
 * 
 * @param problem The SAT problem.
 * @param solution A known satisfying assignment obtained from a prior sat_isSatisfiable call.
 * @param varIndex 0-based index of the variable to query.
 * @return 1 if forced true in all solutions, -1 if forced false, 0 if undetermined.
 */
int8_t sat_getBackboneValue(SAT_Problem* problem, bool* solution, uint32_t varIndex);

/**
 * @brief Returns the current number of clauses in the SAT problem.
 * 
 * @param problem The SAT problem.
 * @return Number of clauses.
 */
size_t sat_getNumClauses(const SAT_Problem* problem);

/**
 * @brief Returns the number of variables in the SAT problem.
 * 
 * @param problem The SAT problem.
 * @return Number of variables.
 */
uint32_t sat_getNumVariables(const SAT_Problem* problem);
