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
#include <stdio.h>

//***************************************************************************//
//**************************  PUBLIC TYPEDEFS *******************************//
//***************************************************************************//

typedef int32_t            SAT_Literal;
typedef struct SAT_Problem SAT_Problem;

/**
 * @brief Represents a variable assignment, resulting from a backbone-test.
 *
 * @var SAT_Assignment::SAT_UNKNOWN Assignment is not forced by the current clause set.
 * @var SAT_Assignment::SAT_FALSE Assignment is forced to false.
 * @var SAT_Assignment::SAT_TRUE Assignment is forced to true.
 */
typedef enum
{
    SAT_UNKNOWN = 0,
    SAT_FALSE,
    SAT_TRUE
} SAT_Assignment;

/**
 * @brief Result of a satisfiability check.
 * 
 * @var SAT_Result::solution Heap-allocated boolean assignment array if satisfiable; NULL otherwise.
 *                           Array size is the number of variables.
 * @var SAT_Result::numLiterals Number of literals in the conflict clause if unsatisfiable; 0 otherwise.
 * @var SAT_Result::conflictClause Pointer to the conflict clause literals if unsatisfiable; NULL otherwise.
 *                                 This is a heap-allocated copy owned by this result.
 */
typedef struct
{
    bool*        solution;
    uint32_t     numLiterals;
    SAT_Literal* conflictClause;
} SAT_Result;

//***************************************************************************//
//**************************  PUBLIC VARIABLE DECLARATIONS ******************//
//***************************************************************************//

/**
 * @brief Frees all memory associated with a SAT problem.
 * 
 * @param problem SAT problem to free.
 */
void sat_freeProblem(SAT_Problem* problem);

/**
 * @brief Allocates and initializes a new SAT problem.
 * 
 * @return Newly allocated SAT problem.
 */
SAT_Problem* sat_initProblem(void);

/**
 * @brief Adds a clause (disjunction of literals) to the SAT problem using a pointer to a literal array.
 * 
 * @param problem SAT problem.
 * @param numLiterals Number of literals in the clause.
 * @param literals Array of literals forming the clause.
 */
void sat_vAddClause(SAT_Problem* problem, uint32_t numLiterals, const SAT_Literal* literals);

/**
 * @brief Adds a clause to the SAT problem using variadic literal arguments.
 * 
 * @param problem SAT problem.
 * @param numLiterals Number of literal arguments that follow.
 */
void sat_addClause(SAT_Problem* problem, uint32_t numLiterals, ...);

/**
 * @brief Adds a unit clause (single literal assertion) to the SAT problem.
 * 
 * @param problem SAT problem.
 * @param literal Literal to assert.
 */
void sat_addUnitClause(SAT_Problem* problem, SAT_Literal literal);

/**
 * @brief Pushes a checkpoint level for temporary clause additions.
 * 
 * @param problem SAT problem.
 */
void sat_pushLevel(SAT_Problem* problem);

/**
 * @brief Commits the top checkpoint level, keeping clauses added since the last push.
 *
 * @param problem SAT problem.
 */
void sat_commitLevel(SAT_Problem* problem);

/**
 * @brief Pops the top checkpoint level and removes clauses added since the matching push.
 * 
 * @param problem SAT problem.
 */
void sat_popLevel(SAT_Problem* problem);

/**
 * @brief Frees a SAT_Result returned by sat_isSatisfiable* APIs.
 *
 * Frees the solution array, conflict-clause copy, and the SAT_Result object itself.
 *
 * @param result SAT result to free.
 */
void sat_freeResult(const SAT_Result* result);

/**
 * @brief Checks satisfiability of the current problem with its current clause set.
 * 
 * @param problem SAT problem.
 * @param outResult If non-NULL, stores a heap-allocated SAT_Result* for both SAT and UNSAT outcomes.
 *                  Caller must free it with sat_freeResult().
 * @return true if satisfiable, false otherwise.
 */
bool sat_isSatisfiable(SAT_Problem* problem, const SAT_Result** outResult);

/**
 * @brief Checks satisfiability with additional temporary literal assumptions.
 * 
 * @param problem SAT problem.
 * @param numAssumptions Number of assumption literals.
 * @param assumptions Array of assumption literals.
 * @param outResult If non-NULL, stores a heap-allocated SAT_Result* for both SAT and UNSAT outcomes.
 *                  Caller must free it with sat_freeResult().
 * @return true if satisfiable, false otherwise.
 */
bool sat_isSatisfiableWithAssumptions(
    SAT_Problem*       problem,
    uint32_t           numAssumptions,
    const SAT_Literal* assumptions,
    const SAT_Result** outResult);

/**
 * @brief Computes the backbone assignment of a variable.
 * 
 * @param problem SAT problem.
 * @param solution A known satisfying assignment obtained from a prior sat_isSatisfiable() call.
 * @param varIndex 0-based index of the variable to query.
 * @return SAT_TRUE if forced true in all solutions, SAT_FALSE if forced false, SAT_UNKNOWN otherwise.
 */
SAT_Assignment sat_getBackboneValue(SAT_Problem* problem, const bool* solution, uint32_t varIndex);

/**
 * @brief Returns the current number of clauses in the SAT problem.
 * 
 * @param problem SAT problem.
 * @return Number of clauses.
 */
size_t sat_getNumClauses(const SAT_Problem* problem);

/**
 * @brief Returns the number of variables in the SAT problem.
 *        The number of variables is computed as the highest variable index encountered
 *        in all clauses added plus one. Initially 0 when the problem is created.
 *        Note that distinct variables are not counted, so if only variable 5 is used in clauses, this function will return 6.
 * 
 * @param problem SAT problem.
 * @return Number of variables (highest variable index + 1).
 */
size_t sat_getNumVariables(const SAT_Problem* problem);

/**
 * @brief Serializes the SAT problem as DIMACS CNF and writes it to a FILE stream.
 *
 * Writes comment lines before the DIMACS problem statement:
 * - c Exported by SAT solver
 * - c sat or c unsat
 * - c backbone <literal> for each backbone literal (when satisfiable)
 *
 * Then writes a DIMACS header of the form "p cnf <numVars> <numClauses>" followed by one
 * line per clause, where each clause is a sequence of integer literals terminated by 0.
 *
 * @param problem SAT problem.
 * @param includeBackbones Whether to include backbone literals as comments.
 * @param out Output stream to write DIMACS CNF text to.
 * @return true if the full serialization was written successfully, false on I/O failure.
 */
bool sat_serializeDimacsCnf(SAT_Problem* problem, bool includeBackbones, FILE* out);
