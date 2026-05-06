/**
 * @file sat.c
 */


//***************************************************************************//
//************************** INCLUDES ***************************************//
//***************************************************************************//

#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

#include "sat.h"
#include "cdcl.h"
#include "sat_util.h"

//***************************************************************************//
//**************************        PRIVATE DEFINES        ******************//
//***************************************************************************//

/** @brief Initial capacity for the clause array of a SAT problem. */
#define INITIAL_CLAUSE_CAPACITY 100u

//***************************************************************************//
//*************************** PUBLIC FUNCTION DEFINITIONS *******************//
//***************************************************************************//

void sat_freeProblem(SAT_Problem* problem)
{
    assert(problem != NULL);
    for(size_t i = 0u; i < problem->numClauses; i++)
    {
        free(problem->clauses[i]);
    }
    free(problem->clauses);
    free(problem);
}

SAT_Problem* sat_initProblem(void)
{
    SAT_Problem* problem = malloc(sizeof(SAT_Problem));
    *problem             = (SAT_Problem){
                    .numVars    = 0u,
                    .numClauses = 0u,
                    .capacity   = INITIAL_CLAUSE_CAPACITY,
                    .clauses    = malloc(INITIAL_CLAUSE_CAPACITY * sizeof(Clause*)),
                    .numLevels  = 0u
    };
    return problem;
}

void sat_vAddClause(SAT_Problem* problem, uint32_t numLiterals, const SAT_Literal* literals)
{
    assert(problem != NULL);
    assert(literals != NULL);

    for(uint32_t i = 0u; i < numLiterals; i++)
    {
        uint32_t varIndex = sat_util_getVar(literals[i]);
        if((varIndex + 1u) > problem->numVars)
        {
            problem->numVars = (varIndex + 1u);
        }
        assert(sat_util_isValidLiteral(problem, literals[i])); // Check must be done after updating numVars
    }

    Clause* clause      = malloc(sizeof(Clause) + (numLiterals * sizeof(SAT_Literal)));
    clause->numLiterals = numLiterals;
    memcpy(clause->literals, literals, numLiterals * sizeof(SAT_Literal));
    sat_util_addClause(problem, clause);
}

void sat_addClause(SAT_Problem* problem, uint32_t numLiterals, ...)
{
    va_list args;
    va_start(args, numLiterals);

    if(numLiterals > 0u)
    {
        SAT_Literal* literals = malloc(numLiterals * sizeof(SAT_Literal));
        for(uint32_t i = 0u; i < numLiterals; i++)
        {
            literals[i] = va_arg(args, SAT_Literal);
        }
        sat_vAddClause(problem, numLiterals, literals);
        free(literals);
    }

    va_end(args);
}

void sat_addUnitClause(SAT_Problem* problem, SAT_Literal literal)
{
    sat_vAddClause(problem, 1u, &literal);
}

void sat_pushLevel(SAT_Problem* problem)
{
    assert(problem != NULL);
    assert(problem->numLevels < CDCL_MAX_CHECKPOINT_LEVELS);
    problem->levels[problem->numLevels++] = problem->numClauses;
}

void sat_commitLevel(SAT_Problem* problem)
{
    assert(problem != NULL);
    assert(problem->numLevels > 0u);
    problem->numLevels--;
}

void sat_popLevel(SAT_Problem* problem)
{
    assert(problem != NULL);
    assert(problem->numLevels > 0u);
    size_t level = problem->levels[--problem->numLevels];
    for(size_t i = level; i < problem->numClauses; i++)
    {
        free((void*)problem->clauses[i]);
    }
    problem->numClauses = level;
}

void sat_freeResult(const SAT_Result* result)
{
    assert(result != NULL);
    free((void*)result->solution);
    free((void*)result->conflictClause);
    free((void*)result);
}

bool sat_isSatisfiable(SAT_Problem* problem, const SAT_Result** outResult)
{
    return sat_isSatisfiableWithAssumptions(problem, 0u, NULL, outResult);
}

bool sat_isSatisfiableWithAssumptions(
    SAT_Problem*       problem,
    uint32_t           numAssumptions,
    const SAT_Literal* assumptions,
    const SAT_Result** outResult)
{
    assert(problem != NULL);
    assert((numAssumptions == 0u) || (assumptions != NULL));

    if(numAssumptions > 0u)
    {
        sat_pushLevel(problem);
    }

    bool isSat = cdcl_isSatisfiableWithAssumptions(problem, numAssumptions, assumptions, outResult);

    if(numAssumptions > 0u)
    {
        sat_popLevel(problem);
    }
    return isSat;
}

SAT_Assignment sat_getBackboneValue(SAT_Problem* problem, const bool* solution, uint32_t varIndex)
{
    assert(problem != NULL);
    assert(varIndex < problem->numVars);

    SAT_Literal testedLit = (SAT_Literal)(varIndex + 1u);
    if(solution[varIndex] == false)
    {
        testedLit = -testedLit;
    }

    SAT_Literal testedNegated = -testedLit;
    bool        isSat         = sat_isSatisfiableWithAssumptions(problem, 1u, &testedNegated, NULL);
    if(isSat == false)
    {
        sat_addUnitClause(problem, testedLit);
        if(solution[varIndex] == false)
        {
            return SAT_FALSE;
        }
        else
        {
            return SAT_TRUE;
        }
    }
    return SAT_UNKNOWN;
}

size_t sat_getNumClauses(const SAT_Problem* problem)
{
    assert(problem != NULL);
    return problem->numClauses;
}

size_t sat_getNumVariables(const SAT_Problem* problem)
{
    assert(problem != NULL);
    return problem->numVars;
}

bool sat_serializeDimacsCnf(SAT_Problem* problem, bool includeBackbones, FILE* out)
{
    assert(problem != NULL);
    assert(out != NULL);

    if(fprintf(out, "c Exported by SAT solver\n") < 0)
    {
        return false;
    }

    const SAT_Result* result = NULL;
    bool              isSat  = sat_isSatisfiable(problem, &result);

    if(fprintf(out, "c %s\n", isSat ? "sat" : "unsat") < 0)
    {
        sat_freeResult(result);
        return false;
    }

    if((isSat == true) && (includeBackbones == true))
    {
        for(size_t varIndex = 0u; varIndex < problem->numVars; varIndex++)
        {
            SAT_Assignment backboneValue = sat_getBackboneValue(problem, result->solution, (uint32_t)varIndex);
            if((backboneValue == SAT_TRUE) || (backboneValue == SAT_FALSE))
            {
                SAT_Literal backboneLiteral = (SAT_Literal)(varIndex + 1u);
                if(backboneValue == SAT_FALSE)
                {
                    backboneLiteral = -backboneLiteral;
                }
                if(fprintf(out, "c backbone %d\n", backboneLiteral) < 0)
                {
                    sat_freeResult(result);
                    return false;
                }
            }
        }
    }

    sat_freeResult(result);

    if(fprintf(out, "p cnf %zu %zu\n", problem->numVars, problem->numClauses) < 0)
    {
        return false;
    }

    for(size_t clauseIndex = 0u; clauseIndex < problem->numClauses; clauseIndex++)
    {
        const Clause* clause = problem->clauses[clauseIndex];
        for(uint32_t literalIndex = 0u; literalIndex < clause->numLiterals; literalIndex++)
        {
            if(fprintf(out, "%d ", clause->literals[literalIndex]) < 0)
            {
                return false;
            }
        }

        if(fprintf(out, "0\n") < 0)
        {
            return false;
        }
    }

    return true;
}
