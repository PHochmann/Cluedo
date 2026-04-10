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
#include <sys/types.h>
#include <stdarg.h>

#include "sat.h"

//***************************************************************************//
//**************************        PRIVATE DEFINES        ******************//
//***************************************************************************//

/** @brief Initial capacity for the clause array of a SAT problem. */
#define INITIAL_CLAUSE_CAPACITY 100u

/** @brief Initial capacity for the solver's propagation trail. */
#define INITIAL_TRAIL_CAPACITY 100u

/** @brief Initial capacity for each literal's watchlist. */
#define INITIAL_WATCHLIST_CAPACITY 10u

/** @brief Maximum number of nested checkpoint (push/pop) levels supported. */
#define MAX_CHECKPOINT_LEVELS 8u

//***************************************************************************//
//************************** PRIVATE TYPEDEFS *******************************//
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
    size_t   levels[MAX_CHECKPOINT_LEVELS];
};

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
//************************** PRIVATE FUNCTION DECLARATIONS ******************//
//***************************************************************************//

/**
 * @brief Ensures that a dynamically growing array has at least the specified capacity, reallocating if necessary.
 *
 * @note The array is grown by doubling its capacity until it meets the newCapacity requirement.
 * 
 * @param array Pointer to the array pointer to grow.
 * @param capacity Pointer to the current capacity of the array.
 * @param newCapacity The desired minimum capacity.
 * @param elementSize Size of each element in the array.
 */
static void ensureCapacity(void** array, size_t* capacity, size_t newCapacity, size_t elementSize);

/**
 * @brief Checks whether a literal is within the valid range for the given problem.
 * 
 * @param problem SAT problem.
 * @param lit Literal to validate.
 * @return true if the literal is valid, false otherwise.
 */
static bool isValidLiteral(const SAT_Problem* problem, SAT_Literal lit);

/**
 * @brief Converts a literal to a unique index into a flat watchlist array.
 *
 * Positive literals map to [0..numVars-1]; negative literals map to [numVars..2*numVars-1].
 * 
 * @param literal Literal to convert.
 * @param numVars Total number of variables.
 * @return Index into the watchlist array.
 */
static size_t getLiteralIndex(SAT_Literal literal, uint32_t numVars);

/**
 * @brief Gets the 0-based variable index for a literal.
 * 
 * @param literal Literal (positive or negative).
 * @return 0-based variable index.
 */
static uint32_t getVar(SAT_Literal literal);

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
 * @brief Appends a clause pointer to the SAT problem's clause array, growing it if necessary.
 * 
 * @param problem SAT problem.
 * @param clause Clause to add.
 */
static void addClause(SAT_Problem* problem, Clause* clause);

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
 * @brief Selects the next unassigned variable and returns it as a positive literal (naive VSIDS-free heuristic).
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
 * @brief Extracts a heap-allocated Boolean solution array from a complete solver assignment.
 * 
 * @param solver The solver with a complete assignment.
 * @return Heap-allocated array of size numVars; solution[i] is true iff variable i+1 is assigned true.
 */
static bool* getSolutionFromCompleteAssignment(const Solver* solver);

//***************************************************************************//
//************************** PRIVATE FUNCTION DEFINITIONS *******************//
//***************************************************************************//

static void ensureCapacity(void** array, size_t* capacity, size_t newCapacity, size_t elementSize)
{
    if(*capacity >= newCapacity)
    {
        return;
    }
    else
    {
        if(*capacity == 0u)
        {
            *capacity = 1u;
        }
        while(*capacity < newCapacity)
        {
            *capacity *= 2u;
        }
        *array = realloc(*array, (*capacity) * elementSize);
    }
}

static bool isValidLiteral(const SAT_Problem* problem, SAT_Literal lit)
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
    return true;
}

// -x ... x -> 0 ... 2 * numVars - 1
static size_t getLiteralIndex(SAT_Literal literal, uint32_t numVars)
{
    return (literal > 0) ? (literal - 1u) : (numVars - literal - 1u);
}

static uint32_t getVar(SAT_Literal literal)
{
    return (literal > 0) ? (literal - 1u) : (-literal - 1u);
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
    return (isAssigned(solver, getVar(literal)) == true) && (getAssignment(solver, getVar(literal))->literal == -literal);
}

static bool isLiteralSatisfied(const Solver* solver, SAT_Literal literal)
{
    return (isAssigned(solver, getVar(literal)) == true) && (getAssignment(solver, getVar(literal))->literal == literal);
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

static void addClause(SAT_Problem* problem, Clause* clause)
{
    ensureCapacity((void**)&problem->clauses, &problem->capacity, problem->numClauses + 1u, sizeof(Clause*));
    problem->clauses[problem->numClauses++] = clause;
}

static void addToWatchlist(Watchlist* watchlist, Clause* clause)
{
    ensureCapacity((void**)&watchlist->clauses, &watchlist->capacity, watchlist->numClauses + 1u, sizeof(Clause*));
    watchlist->clauses[watchlist->numClauses++] = clause;
}

static void pushToTrail(Solver* solver, SAT_Literal literal, const Clause* reason)
{
    uint32_t index = solver->trail.trailSize;
    ensureCapacity((void**)&solver->trail.trail, &solver->trail.capacity, index + 1u, sizeof(TrailElement));

    solver->assignment[getVar(literal)] = index;
    solver->trail.trail[index]          = (TrailElement){
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
            return clause; // There was another unit clause that contained assertedLit -> Two contradicting unit clauses in set
        }

        size_t      litIndex      = (clause->literals[0] == -assertedLit) ? 0 : 1;
        size_t      otherLitIndex = 1 - litIndex;
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
                // Found a replacement literal to watch, update the clause and watchlists accordingly
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
        } // Don't increment i when clause was deleted because a new one will takes its place
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
    return 0; // No unassigned variables left
}

static uint32_t analyzeConflict(Solver* solver, const Clause* conflictClause, Clause** outLearnedClause)
{
    const Trail* trail     = &solver->trail;
    uint32_t     currLevel = solver->currentDecisionLevel;

    bool*   seen         = calloc(solver->problem->numVars, sizeof(bool));
    Clause* learned      = malloc(sizeof(Clause) + trail->trailSize * sizeof(SAT_Literal));
    learned->numLiterals = 1; // UIP will be at position 0

    uint32_t      unwindCount   = 0u;
    uint32_t      backjumpLevel = 0u;
    uint32_t      index         = trail->trailSize;
    const Clause* clause        = conflictClause;
    bool          first         = true;

    while(true)
    {
        for(uint32_t i = 0u; i < clause->numLiterals; i++)
        {
            uint32_t v = getVar(clause->literals[i]);
            if(seen[v] == false)
            {
                seen[v]     = true;
                uint32_t dl = getAssignment(solver, v)->decisionLevel;
                if(dl == currLevel)
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

        if(first == false)
        {
            unwindCount--;
            seen[getVar(trail->trail[index].literal)] = false;
        }
        first = false;

        while(seen[getVar(trail->trail[--index].literal)] == false)
            ;

        if(unwindCount == 1u)
        {
            break;
        }
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
        solver->assignment[getVar(te->literal)] = -1;
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
        if(conflict != NULL) // Conflict detected
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
            if(isAssigned(solver, getVar(clause->literals[0])) == false)
            {
                pushToTrail(solver, clause->literals[0], clause);
            }
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
                return false; // Unsatisfiable
            }

            Clause*  learnedClause;
            uint32_t backtrackLevel = analyzeConflict(solver, conflict, &learnedClause);
            addClause(solver->problem, learnedClause); // Permanently add the learned clause to the problem
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

static bool* getSolutionFromCompleteAssignment(const Solver* solver)
{
    bool* result = malloc(solver->problem->numVars * sizeof(bool));
    for(uint32_t i = 0u; i < solver->problem->numVars; i++)
    {
        assert(isAssigned(solver, i));
        result[i] = (getAssignment(solver, i)->literal > 0);
    }
    return result;
}


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

SAT_Problem* sat_initProblem(uint16_t numVars)
{
    assert(numVars > 0u);
    SAT_Problem* problem = malloc(sizeof(SAT_Problem));
    *problem             = (SAT_Problem){
                    .numVars    = numVars,
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
    assert(numLiterals > 0u);
    assert(literals != NULL);

    for(uint32_t i = 0u; i < numLiterals; i++)
    {
        assert(isValidLiteral(problem, literals[i]));
    }

    Clause* clause      = malloc(sizeof(Clause) + (numLiterals * sizeof(SAT_Literal)));
    clause->numLiterals = numLiterals;
    memcpy(clause->literals, literals, numLiterals * sizeof(SAT_Literal));
    addClause(problem, clause);
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
    assert(problem->numLevels < MAX_CHECKPOINT_LEVELS);
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
        free(problem->clauses[i]);
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

    if(outResult != NULL)
    {
        *outResult = NULL;
    }

    Solver solver;
    getSolver(problem, &solver);

    for(uint32_t i = 0u; i < numAssumptions; i++)
    {
        pushToTrail(&solver, assumptions[i], NULL);
    }
    if(numAssumptions > 0u)
    {
        sat_pushLevel(problem);
    }

    const Clause* conflict;
    bool          result = isSatisfiable(&solver, &conflict);

    if(outResult != NULL)
    {
        SAT_Result* resultObj = malloc(sizeof(SAT_Result));

        if(result == true)
        {
            *resultObj = (SAT_Result){
                .solution       = getSolutionFromCompleteAssignment(&solver),
                .numLiterals    = 0u,
                .conflictClause = NULL
            };
        }
        else
        {
            SAT_Literal* conflictClauseCopy = malloc(conflict->numLiterals * sizeof(SAT_Literal));
            memcpy(conflictClauseCopy, conflict->literals, conflict->numLiterals * sizeof(SAT_Literal));

            *resultObj = (SAT_Result){
                .solution       = NULL,
                .numLiterals    = conflict->numLiterals,
                .conflictClause = conflictClauseCopy
            };
        }

        *outResult = resultObj;
    }

    freeSolver(&solver);
    if(numAssumptions > 0u)
    {
        sat_popLevel(problem);
    }
    return result;
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

uint32_t sat_getNumVariables(const SAT_Problem* problem)
{
    assert(problem != NULL);
    return problem->numVars;
}
