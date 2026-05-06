/**
 * @file cdcl.c
 */


//***************************************************************************//
//************************** INCLUDES ***************************************//
//***************************************************************************//

#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>

#include "cdcl.h"
#include "sat_util.h"

//***************************************************************************//
//**************************  PRIVATE DEFINES *******************************//
//***************************************************************************//

/** @brief Initial capacity for the solver's propagation trail. */
#define INITIAL_TRAIL_CAPACITY 100u

/** @brief Initial capacity for each literal's watchlist. */
#define INITIAL_WATCHLIST_CAPACITY 10u

//***************************************************************************//
//************************** PRIVATE TYPEDEFS *******************************//
//***************************************************************************//

/**
 * @brief A single element on the DPLL propagation trail.
 *
 * @var TrailElement::literal The asserted literal.
 * @var TrailElement::decisionLevel Decision level at which the literal was asserted.
 * @var TrailElement::reason Pointer to the clause that caused propagation, or NULL if this is a decision.
 */
typedef struct
{
    SAT_Literal   literal;
    uint32_t      decisionLevel;
    const Clause* reason;
} TrailElement;

/**
 * @brief The assignment trail used during DPLL solving.
 *
 * @var Trail::trailSize Current number of elements on the trail.
 * @var Trail::capacity Allocated capacity of the trail array.
 * @var Trail::head Index of the next unprocessed element for unit propagation.
 * @var Trail::trail Heap-allocated array of trail elements.
 */
typedef struct
{
    size_t        trailSize;
    size_t        capacity;
    size_t        head;
    TrailElement* trail;
} Trail;

/**
 * @brief A two-literal watchlist entry for a single literal: the set of clauses watched on this literal.
 *
 * @var Watchlist::numClauses Number of clauses currently in the watchlist.
 * @var Watchlist::capacity Allocated capacity of the clause pointer array.
 * @var Watchlist::clauses Heap-allocated array of pointers to watched clauses.
 */
typedef struct
{
    size_t   numClauses;
    size_t   capacity;
    Clause** clauses;
} Watchlist;

/**
 * @brief Holds the internal state of the CDCL solver for a single satisfiability check.
 *
 * @var Solver::problem Pointer to the SAT problem being solved.
 * @var Solver::currentDecisionLevel Current decision level.
 * @var Solver::watchlists Array of watchlists, one per literal (indexed by literal index).
 * @var Solver::assignment Maps each variable to its trail index, or -1 if unassigned.
 * @var Solver::trail The propagation trail.
 */
typedef struct
{
    SAT_Problem* problem;
    uint32_t     currentDecisionLevel;
    Watchlist*   watchlists;
    ssize_t*     assignment;
    Trail        trail;
} Solver;

//***************************************************************************//
//************************** PRIVATE VARIABLE DECLARATIONS ******************//
//***************************************************************************//


//***************************************************************************//
//************************** PRIVATE FUNCTION DECLARATIONS ******************//
//***************************************************************************//

/**
 * @brief Converts a literal to a unique index into a flat watchlist array.
 *
 * @param literal Literal to convert.
 * @param numVars Total number of variables.
 * @return Index into the watchlist array.
 */
static size_t getLiteralIndex(SAT_Literal literal, uint32_t numVars);

/**
 * @brief Checks whether a variable has been assigned a value in the current solver state.
 *
 * @param solver Solver state.
 * @param varIndex 0-based variable index.
 * @return true if the variable is assigned, false if unassigned.
 */
static bool isAssigned(const Solver* solver, uint32_t varIndex);

/**
 * @brief Returns the trail element for the assignment of a variable.
 *
 * @param solver Solver state.
 * @param varIndex 0-based variable index (must be assigned).
 * @return Pointer to the trail element recording the assignment.
 */
static TrailElement* getAssignment(const Solver* solver, uint32_t varIndex);

/**
 * @brief Checks whether a literal is falsified by the current assignment.
 *
 * @param solver Solver state.
 * @param literal Literal to check.
 * @return true if the literal's variable is assigned the opposite polarity.
 */
static bool isLiteralFalsified(const Solver* solver, SAT_Literal literal);

/**
 * @brief Checks whether a literal is satisfied by the current assignment.
 *
 * @param solver Solver state.
 * @param literal Literal to check.
 * @return true if the literal's variable is assigned the same polarity.
 */
static bool isLiteralSatisfied(const Solver* solver, SAT_Literal literal);

/**
 * @brief Gets the watchlist for a given literal.
 *
 * @param solver Solver state.
 * @param literal Literal whose watchlist to retrieve.
 * @return Pointer to the watchlist for the given literal.
 */
static Watchlist* getWatchlist(const Solver* solver, SAT_Literal literal);

/**
 * @brief Removes the clause at a given index from a watchlist (order not preserved).
 *
 * @param watchlist Watchlist to modify.
 * @param index Index of the clause to remove.
 */
static void removeFromWatchlist(Watchlist* watchlist, size_t index);

/**
 * @brief Appends a clause to a watchlist, growing it if necessary.
 *
 * @param watchlist Watchlist to append to.
 * @param clause Clause to watch.
 */
static void addToWatchlist(Watchlist* watchlist, Clause* clause);

/**
 * @brief Pushes a literal onto the propagation trail and records its reason clause.
 *
 * @param solver Solver state.
 * @param literal Literal being asserted.
 * @param reason Clause that forced this propagation, or NULL if it is a decision.
 */
static void pushToTrail(Solver* solver, SAT_Literal literal, const Clause* reason);

/**
 * @brief Updates the two-watched-literal scheme after a literal has been asserted, performing unit
 *        propagation and detecting conflicts.
 *
 * @param solver Solver state.
 * @param assertedLit Literal that was just asserted.
 * @return Pointer to the conflicting clause if a conflict is detected, NULL otherwise.
 */
static Clause* updateWatchlists(Solver* solver, SAT_Literal assertedLit);

/**
 * @brief Selects the next unassigned variable and returns it as a positive literal.
 *
 * @param solver Solver state.
 * @return A positive literal for the next decision variable, or 0 if all variables are assigned.
 */
static SAT_Literal getNextDecision(const Solver* solver);

/**
 * @brief Performs 1-UIP conflict analysis, produces a learned clause, and returns the backjump level.
 *
 * @param solver Solver state.
 * @param conflictClause Clause that triggered the conflict.
 * @param outLearnedClause Output: heap-allocated learned clause (caller must add it to the problem).
 * @return The decision level to backjump to.
 */
static uint32_t analyzeConflict(Solver* solver, const Clause* conflictClause, Clause** outLearnedClause);

/**
 * @brief Backtracks the solver to the given decision level, undoing all trail assignments above it.
 *
 * @param solver Solver state.
 * @param backtrackLevel The decision level to backtrack to.
 */
static void backtrack(Solver* solver, uint32_t backtrackLevel);

/**
 * @brief Performs exhaustive unit propagation and returns the first conflicting clause if found.
 *
 * @param solver Solver state.
 * @return Pointer to a conflicting clause if a conflict is found, NULL if propagation succeeded.
 */
static Clause* unitPropagate(Solver* solver);

/**
 * @brief Runs the CDCL satisfiability check on the given solver state.
 *
 * @param solver The solver.
 * @param outConflict If non-NULL and unsatisfiable, receives a pointer to the final conflict clause.
 * @return true if the problem is satisfiable, false otherwise.
 */
static bool isSatisfiable(Solver* solver, const Clause** outConflict);

/**
 * @brief Initializes a Solver from a SAT_Problem, building watchlists and setting up internal state.
 *
 * @param problem The SAT problem to solve.
 * @param outSolver Output: initialized solver ready for use.
 */
static void getSolver(SAT_Problem* problem, Solver* outSolver);

/**
 * @brief Frees all heap memory owned by a solver (watchlists and trail, but not the problem).
 *
 * @param solver The solver to free.
 */
static void freeSolver(Solver* solver);

/**
 * @brief Constructs a heap-allocated SAT_Result object to be returned to the user.
 *
 * @param solver The solver after isSatisfiable() has been called.
 * @param isSat Return value of isSatisfiable() indicating whether the problem was satisfiable.
 * @param conflict The conflicting clause returned by isSatisfiable(), if isSat is false, otherwise NULL.
 * @return Heap-allocated SAT_Result structure containing the solution or conflict information.
 */
static SAT_Result* getResult(const Solver* solver, bool isSat, const Clause* conflict);

//***************************************************************************//
//************************** PRIVATE FUNCTION DEFINITIONS *******************//
//***************************************************************************//

static size_t getLiteralIndex(SAT_Literal literal, uint32_t numVars)
{
    return (literal > 0) ? (literal - 1u) : (numVars - literal - 1u);
}

static bool isAssigned(const Solver* solver, uint32_t varIndex)
{
    return solver->assignment[varIndex] != -1;
}

static TrailElement* getAssignment(const Solver* solver, uint32_t varIndex)
{
    return &solver->trail.trail[solver->assignment[varIndex]];
}

static bool isLiteralFalsified(const Solver* solver, SAT_Literal literal)
{
    return (isAssigned(solver, sat_util_getVar(literal)) == true) &&
           (getAssignment(solver, sat_util_getVar(literal))->literal == -literal);
}

static bool isLiteralSatisfied(const Solver* solver, SAT_Literal literal)
{
    return (isAssigned(solver, sat_util_getVar(literal)) == true) &&
           (getAssignment(solver, sat_util_getVar(literal))->literal == literal);
}

static Watchlist* getWatchlist(const Solver* solver, SAT_Literal literal)
{
    return &solver->watchlists[getLiteralIndex(literal, solver->problem->numVars)];
}

static void removeFromWatchlist(Watchlist* watchlist, size_t index)
{
    assert(index < watchlist->numClauses);
    watchlist->clauses[index] = watchlist->clauses[--watchlist->numClauses];
}

static void addToWatchlist(Watchlist* watchlist, Clause* clause)
{
    sat_util_ensureCapacity((void**)&watchlist->clauses, &watchlist->capacity, watchlist->numClauses + 1u, sizeof(Clause*));
    watchlist->clauses[watchlist->numClauses++] = clause;
}

static void pushToTrail(Solver* solver, SAT_Literal literal, const Clause* reason)
{
    uint32_t index = solver->trail.trailSize;
    sat_util_ensureCapacity((void**)&solver->trail.trail, &solver->trail.capacity, index + 1u, sizeof(TrailElement));

    solver->assignment[sat_util_getVar(literal)] = index;
    solver->trail.trail[index]                   = (TrailElement){
                          .literal       = literal,
                          .decisionLevel = solver->currentDecisionLevel,
                          .reason        = reason
    };
    solver->trail.trailSize++;
}

static Clause* updateWatchlists(Solver* solver, SAT_Literal assertedLit)
{
    Watchlist* watchlist = getWatchlist(solver, -assertedLit);
    size_t     i         = 0u;
    while(i < watchlist->numClauses)
    {
        Clause* clause = watchlist->clauses[i];
        if(clause->numLiterals == 1u)
        {
            return clause;
        }

        size_t      litIndex      = (clause->literals[0] == -assertedLit) ? 0u : 1u;
        size_t      otherLitIndex = 1u - litIndex;
        SAT_Literal otherLit      = clause->literals[otherLitIndex];

        if(isLiteralSatisfied(solver, otherLit) == true)
        {
            i++;
            continue;
        }

        bool replacementFound = false;
        for(size_t j = 2u; j < clause->numLiterals; j++)
        {
            if(isLiteralFalsified(solver, clause->literals[j]) == false)
            {
                clause->literals[litIndex] = clause->literals[j];
                clause->literals[j]        = -assertedLit;
                addToWatchlist(getWatchlist(solver, clause->literals[litIndex]), clause);
                removeFromWatchlist(watchlist, i);
                replacementFound = true;
                break;
            }
        }

        if(replacementFound == false)
        {
            if(isLiteralFalsified(solver, otherLit) == true)
            {
                return clause;
            }
            else
            {
                pushToTrail(solver, otherLit, clause);
            }
            i++;
        }
    }
    return NULL;
}

static SAT_Literal getNextDecision(const Solver* solver)
{
    for(uint32_t i = 0u; i < solver->problem->numVars; i++)
    {
        if(isAssigned(solver, i) == false)
        {
            return (SAT_Literal)(i + 1u);
        }
    }
    return 0;
}

static uint32_t analyzeConflict(Solver* solver, const Clause* conflictClause, Clause** outLearnedClause)
{
    const Trail* trail   = &solver->trail;
    bool*        seen    = calloc(solver->problem->numVars, sizeof(bool));
    Clause*      learned = malloc(sizeof(Clause) + trail->trailSize * sizeof(SAT_Literal));
    learned->numLiterals = 1u;

    uint32_t      unwindCount   = 0u;
    uint32_t      backjumpLevel = 0u;
    uint32_t      index         = trail->trailSize;
    const Clause* clause        = conflictClause;

    while(unwindCount != 1u)
    {
        for(uint32_t i = 0u; i < clause->numLiterals; i++)
        {
            uint32_t v = sat_util_getVar(clause->literals[i]);
            if(seen[v] == false)
            {
                seen[v]     = true;
                uint32_t dl = getAssignment(solver, v)->decisionLevel;
                if(dl == solver->currentDecisionLevel)
                {
                    unwindCount++;
                }
                else
                {
                    learned->literals[learned->numLiterals++] = clause->literals[i];
                    if(dl > backjumpLevel)
                    {
                        backjumpLevel = dl;
                    }
                }
            }
        }

        if(clause != conflictClause)
        {
            seen[sat_util_getVar(trail->trail[index].literal)] = false;
            unwindCount--;
        }

        do
        {
            index--;
        } while(seen[sat_util_getVar(trail->trail[index].literal)] == false);
        clause = trail->trail[index].reason;
    }

    learned->literals[0] = -trail->trail[index].literal;
    free(seen);
    learned           = realloc(learned, sizeof(Clause) + learned->numLiterals * sizeof(SAT_Literal));
    *outLearnedClause = learned;
    return backjumpLevel;
}

static void backtrack(Solver* solver, uint32_t backtrackLevel)
{
    while(true)
    {
        const TrailElement* te = &solver->trail.trail[solver->trail.trailSize - 1u];
        if(te->decisionLevel == backtrackLevel)
        {
            break;
        }
        solver->assignment[sat_util_getVar(te->literal)] = -1;
        solver->trail.trailSize--;
    }
    solver->trail.head           = solver->trail.trailSize;
    solver->currentDecisionLevel = backtrackLevel;
}

static Clause* unitPropagate(Solver* solver)
{
    while(solver->trail.head < solver->trail.trailSize)
    {
        Clause* conflict = updateWatchlists(solver, solver->trail.trail[solver->trail.head++].literal);
        if(conflict != NULL)
        {
            return conflict;
        }
    }
    return NULL;
}

static bool isSatisfiable(Solver* solver, const Clause** outConflict)
{
    for(size_t i = 0u; i < solver->problem->numClauses; i++)
    {
        const Clause* clause = solver->problem->clauses[i];

        if(clause->numLiterals == 1u)
        {
            if(isAssigned(solver, sat_util_getVar(clause->literals[0])) == false)
            {
                pushToTrail(solver, clause->literals[0], clause);
            }
        }

        if(clause->numLiterals == 0u)
        {
            if(outConflict != NULL)
            {
                *outConflict = clause;
            }
            return false;
        }
    }

    while(true)
    {
        Clause* conflict = unitPropagate(solver);
        if(conflict != NULL)
        {
            if(solver->currentDecisionLevel == 0u)
            {
                if(outConflict != NULL)
                {
                    *outConflict = conflict;
                }
                return false;
            }

            Clause*  learnedClause;
            uint32_t backtrackLevel = analyzeConflict(solver, conflict, &learnedClause);
            sat_util_addClause(solver->problem, learnedClause);
            addToWatchlist(getWatchlist(solver, learnedClause->literals[0]), learnedClause);
            if(learnedClause->numLiterals >= 2u)
            {
                addToWatchlist(getWatchlist(solver, learnedClause->literals[1]), learnedClause);
            }
            backtrack(solver, backtrackLevel);
            pushToTrail(solver, learnedClause->literals[0], learnedClause);
            continue;
        }

        SAT_Literal decisionLiteral = getNextDecision(solver);
        if(decisionLiteral == 0)
        {
            return true;
        }
        solver->currentDecisionLevel++;
        pushToTrail(solver, decisionLiteral, NULL);
    }
}

static void getSolver(SAT_Problem* problem, Solver* outSolver)
{
    *outSolver = (Solver){
        .problem              = problem,
        .currentDecisionLevel = 0u,
        .watchlists           = malloc(2u * problem->numVars * sizeof(Watchlist)),
        .assignment           = calloc(problem->numVars, sizeof(ssize_t)),
        .trail                = {
                           .trailSize = 0u,
                           .capacity  = INITIAL_TRAIL_CAPACITY,
                           .trail     = malloc(INITIAL_TRAIL_CAPACITY * sizeof(TrailElement)) }
    };

    for(size_t i = 0u; i < problem->numVars; i++)
    {
        outSolver->assignment[i] = -1;
    }

    for(size_t i = 0u; i < (2u * problem->numVars); i++)
    {
        outSolver->watchlists[i] = (Watchlist){
            .numClauses = 0u,
            .capacity   = INITIAL_WATCHLIST_CAPACITY,
            .clauses    = malloc(INITIAL_WATCHLIST_CAPACITY * sizeof(Clause*))
        };
    }

    for(size_t i = 0u; i < problem->numClauses; i++)
    {
        Clause* clause = problem->clauses[i];
        if(clause->numLiterals >= 1u)
        {
            addToWatchlist(getWatchlist(outSolver, clause->literals[0]), clause);
        }
        if(clause->numLiterals >= 2u)
        {
            addToWatchlist(getWatchlist(outSolver, clause->literals[1]), clause);
        }
    }
}

static void freeSolver(Solver* solver)
{
    for(size_t i = 0u; i < (2u * solver->problem->numVars); i++)
    {
        free(solver->watchlists[i].clauses);
    }
    free(solver->watchlists);
    free(solver->assignment);
    free(solver->trail.trail);
}

static SAT_Result* getResult(const Solver* solver, bool isSat, const Clause* conflict)
{
    SAT_Result* result = malloc(sizeof(SAT_Result));

    if(isSat == true)
    {
        bool* solution = malloc(solver->problem->numVars * sizeof(bool));
        for(uint32_t i = 0u; i < solver->problem->numVars; i++)
        {
            assert(isAssigned(solver, i));
            solution[i] = (getAssignment(solver, i)->literal > 0);
        }

        *result = (SAT_Result){
            .solution       = solution,
            .numLiterals    = 0u,
            .conflictClause = NULL
        };
    }
    else
    {
        SAT_Literal* conflictClauseCopy = malloc(conflict->numLiterals * sizeof(SAT_Literal));
        memcpy(conflictClauseCopy, conflict->literals, conflict->numLiterals * sizeof(SAT_Literal));

        *result = (SAT_Result){
            .solution       = NULL,
            .numLiterals    = conflict->numLiterals,
            .conflictClause = conflictClauseCopy
        };
    }

    return result;
}

//***************************************************************************//
//************************** PUBLIC FUNCTION DEFINITIONS ********************//
//***************************************************************************//

bool cdcl_isSatisfiableWithAssumptions(
    SAT_Problem*       problem,
    uint32_t           numAssumptions,
    const SAT_Literal* assumptions,
    const SAT_Result** outResult)
{
    assert(problem != NULL);
    assert((numAssumptions == 0u) || (assumptions != NULL));

    Solver solver;
    getSolver(problem, &solver);

    for(uint32_t i = 0u; i < numAssumptions; i++)
    {
        assert(sat_util_isValidLiteral(problem, assumptions[i]));
        pushToTrail(&solver, assumptions[i], NULL);
    }

    const Clause* conflict = NULL;
    bool          isSat    = isSatisfiable(&solver, &conflict);

    if(outResult != NULL)
    {
        *outResult = getResult(&solver, isSat, conflict);
    }

    freeSolver(&solver);
    return isSat;
}
