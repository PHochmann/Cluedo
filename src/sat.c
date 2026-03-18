/**
 * @file sat.c
 */


//***************************************************************************//
//************************** INCLUDES ***************************************//
//***************************************************************************//

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <stdarg.h>

#include "sat.h"

//***************************************************************************//
//************************** PRIVATE TYPEDEFS *******************************//
//***************************************************************************//

/** @brief Todo */
#define INITIAL_CLAUSE_CAPACITY 100u

/** @brief Todo */
#define INITIAL_TRAIL_CAPACITY 100u

/** @brief Todo */
#define INITIAL_WATCHLIST_CAPACITY 10u

/** @brief Todo */
#define MAX_CHECKPOINT_LEVELS 8u

/**
 * @brief Todo
 * 
 * @var Clause::numLiterals Todo
 * @var Clause::literals Todo
 */
typedef struct
{
    uint32_t    numLiterals;
    SAT_Literal literals[];
} Clause;

/**
 * @brief Todo
 * 
 * @var SAT_Problem::numVars Todo
 * @var SAT_Problem::numClauses Todo
 * @var SAT_Problem::capacity Todo
 * @var SAT_Problem::clauses Todo
 * @var SAT_Problem::numLevels Todo
 * @var SAT_Problem::levels Todo
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
 * @brief Todo
 * 
 * @var TrailElement::literal Todo
 * @var TrailElement::decisionLevel Todo
 * @var TrailElement::reason Todo
 */
typedef struct
{
    SAT_Literal   literal;
    uint32_t      decisionLevel;
    const Clause* reason; // NULL if decision
} TrailElement;

/**
 * @brief Todo
 * 
 * @var Trail::trailSize Todo
 * @var Trail::capacity Todo
 * @var Trail::head Todo
 * @var Trail::trail Todo
 */
typedef struct
{
    size_t        trailSize;
    size_t        capacity;
    size_t        head;
    TrailElement* trail;
} Trail;

/**
 * @brief Todo
 * 
 * @var Watchlist::numClauses Todo
 * @var Watchlist::capacity Todo
 * @var Watchlist::clauses Todo
 */
typedef struct
{
    size_t   numClauses;
    size_t   capacity;
    Clause** clauses;
} Watchlist;

/**
 * @brief Todo
 * 
 * @var Solver::problem Todo
 * @var Solver::currentDecisionLevel Todo
 * @var Solver::watchlists Todo
 * @var Solver::assignment Todo
 * @var Solver::trail Todo
 */
typedef struct
{
    SAT_Problem* problem;
    uint32_t     currentDecisionLevel;
    Watchlist*   watchlists;
    ssize_t*     assignment; // -1 = unassigned
    Trail        trail;
} Solver;

//***************************************************************************//
//************************** PRIVATE FUNCTION DECLARATIONS ******************//
//***************************************************************************//

/**
 * @brief Todo
 * 
 * @param problem Todo
 * @param lit Todo
 * @return Todo
 */
static bool isValidLiteral(SAT_Problem* problem, SAT_Literal lit);

/**
 * @brief Todo
 * 
 * @param literal Todo
 * @param numVars Todo
 * @return Todo
 */
static size_t getLiteralIndex(SAT_Literal literal, uint32_t numVars);

/**
 * @brief Todo
 * 
 * @param literal Todo
 * @return Todo
 */
static uint32_t getVar(SAT_Literal literal);

/**
 * @brief Todo
 * 
 * @param solver Todo
 * @param varIndex Todo
 * @return Todo
 */
static bool isAssigned(const Solver* solver, uint32_t varIndex);

/**
 * @brief Todo
 * 
 * @param solver Todo
 * @param varIndex Todo
 * @return Todo
 */
static TrailElement* getAssignment(const Solver* solver, uint32_t varIndex);

/**
 * @brief Todo
 * 
 * @param solver Todo
 * @param literal Todo
 * @return Todo
 */
static bool isLiteralFalsified(const Solver* solver, SAT_Literal literal);

/**
 * @brief Todo
 * 
 * @param solver Todo
 * @param literal Todo
 * @return Todo
 */
static bool isLiteralSatisfied(const Solver* solver, SAT_Literal literal);

/**
 * @brief Todo
 * 
 * @param solver Todo
 * @param literal Todo
 * @return Todo
 */
static Watchlist* getWatchlist(const Solver* solver, SAT_Literal literal);

/**
 * @brief Todo
 * 
 * @param watchlist Todo
 * @param index Todo
 */
static void removeFromWatchlist(Watchlist* watchlist, size_t index);

/**
 * @brief Todo
 * 
 * @param problem Todo
 * @param clause Todo
 */
static void addClause(SAT_Problem* problem, Clause* clause);

/**
 * @brief Todo
 * 
 * @param watchlist Todo
 * @param clause Todo
 */
static void addToWatchlist(Watchlist* watchlist, Clause* clause);

/**
 * @brief Todo
 * 
 * @param solver Todo
 * @param literal Todo
 * @param reason Todo
 */
static void pushToTrail(Solver* solver, SAT_Literal literal, const Clause* reason);

/**
 * @brief Todo
 * 
 * @param solver Todo
 * @param assertedLit Todo
 * @return Todo
 */
static Clause* updateWatchlists(Solver* solver, SAT_Literal assertedLit);

/**
 * @brief Todo
 * 
 * @param solver Todo
 * @return Todo
 */
static SAT_Literal getNextDecision(const Solver* solver);

/**
 * @brief Todo
 * 
 * @param solver Todo
 * @param conflictClause Todo
 * @param outLearnedClause Todo
 * @return Todo
 */
static uint32_t analyzeConflict(Solver* solver, const Clause* conflictClause, Clause** outLearnedClause);

/**
 * @brief Todo
 * 
 * @param solver Todo
 * @param backtrackLevel Todo
 */
static void backtrack(Solver* solver, uint32_t backtrackLevel);

/**
 * @brief Todo
 * 
 * @param solver Todo
 * @return Todo
 */
static Clause* unitPropagate(Solver* solver);

/**
 * @brief Todo
 * 
 * @param solver Todo
 * @param outConflict Todo
 * @return Todo
 */
static bool isSatisfiable(Solver* solver, const Clause** outConflict);

/**
 * @brief Todo
 * 
 * @param problem Todo
 * @param outSolver Todo
 */
static void getSolver(SAT_Problem* problem, Solver* outSolver);

/**
 * @brief Todo
 * 
 * @param solver Todo
 */
static void freeSolver(Solver* solver);

/**
 * @brief Todo
 * 
 * @param solver Todo
 * @return Todo
 */
static bool* getSolutionFromCompleteAssignment(const Solver* solver);

//***************************************************************************//
//************************** PRIVATE FUNCTION DEFINITIONS *******************//
//***************************************************************************//

static bool isValidLiteral(SAT_Problem* problem, SAT_Literal lit)
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
    if(watchlist->numClauses == 0u)
    {
        return;
    }
    watchlist->clauses[index] = watchlist->clauses[--watchlist->numClauses];
}

static void addClause(SAT_Problem* problem, Clause* clause)
{
    if(problem->numClauses == problem->capacity)
    {
        problem->capacity *= 2u;
        problem->clauses = realloc(problem->clauses, problem->capacity * sizeof(Clause*));
    }
    problem->clauses[problem->numClauses++] = clause;
}

static void addToWatchlist(Watchlist* watchlist, Clause* clause)
{
    if(watchlist->numClauses == watchlist->capacity)
    {
        watchlist->capacity *= 2u;
        watchlist->clauses = realloc(watchlist->clauses, watchlist->capacity * sizeof(Clause*));
    }
    watchlist->clauses[watchlist->numClauses++] = clause;
}

static void pushToTrail(Solver* solver, SAT_Literal literal, const Clause* reason)
{
    uint32_t index = solver->trail.trailSize;
    if(index == solver->trail.capacity)
    {
        solver->trail.capacity *= 2u;
        solver->trail.trail = realloc(solver->trail.trail, solver->trail.capacity * sizeof(TrailElement));
    }
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
        Clause* clause = solver->problem->clauses[i];
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
        .assignment           = calloc(problem->numVars, sizeof(TrailElement*)),
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
        if(isAssigned(solver, i) == false)
        {
            // Assignment not complete!
            free(result);
            return NULL;
        }
        result[i] = (getAssignment(solver, i)->literal > 0);
    }
    return result;
}


//***************************************************************************//
//*************************** PUBLIC FUNCTION DEFINITIONS *******************//
//***************************************************************************//

void sat_freeProblem(SAT_Problem* problem)
{
    for(size_t i = 0u; i < problem->numClauses; i++)
    {
        free(problem->clauses[i]);
    }
    free(problem->clauses);
    free(problem);
}

SAT_Problem* sat_initProblem(uint16_t numVars)
{
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
    for(uint32_t i = 0u; i < numLiterals; i++)
    {
        if(isValidLiteral(problem, literals[i]) == false)
        {
            printf("%s:%d: Invalid literal %d\n", __FILE__, __LINE__, literals[i]);
        }
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

    // Allocate and collect literals
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

bool sat_pushLevel(SAT_Problem* problem)
{
    if(problem->numLevels >= MAX_CHECKPOINT_LEVELS)
    {
        return false;
    }
    problem->levels[problem->numLevels++] = problem->numClauses;
    return true;
}

bool sat_popLevel(SAT_Problem* problem)
{
    if(problem->numLevels == 0u)
    {
        return false;
    }
    size_t level = problem->levels[--problem->numLevels];
    for(size_t i = level; i < problem->numClauses; i++)
    {
        free(problem->clauses[i]);
    }
    problem->numClauses = level;
    return true;
}

bool sat_isSatisfiable(SAT_Problem* problem, bool** solution, uint32_t* outNumLiterals, const SAT_Literal** outConflictClause)
{
    Solver solver;
    getSolver(problem, &solver);
    const Clause* conflict;
    bool          result = isSatisfiable(&solver, &conflict);
    if((result == true) && (solution != NULL))
    {
        *solution = getSolutionFromCompleteAssignment(&solver);
    }
    if(result == false)
    {
        if(outNumLiterals != NULL)
        {
            *outNumLiterals = conflict->numLiterals;
        }
        if(outConflictClause != NULL)
        {
            *outConflictClause = conflict->literals;
        }
    }
    freeSolver(&solver);
    return result;
}

bool sat_isSatisfiableWithAssumptions(
    SAT_Problem*        problem,
    uint32_t            numAssumptions,
    const SAT_Literal*  assumptions,
    bool**              solution,
    uint32_t*           outNumLiterals,
    const SAT_Literal** outConflictClause)
{
    Solver solver;
    getSolver(problem, &solver);

    for(uint32_t i = 0u; i < numAssumptions; i++)
    {
        pushToTrail(&solver, assumptions[i], NULL);
    }

    const Clause* conflict;
    bool          result = isSatisfiable(&solver, &conflict);
    if((result == true) && (solution != NULL))
    {
        *solution = getSolutionFromCompleteAssignment(&solver);
    }
    if(result == false)
    {
        if(outNumLiterals != NULL)
        {
            *outNumLiterals = conflict->numLiterals;
        }
        if(outConflictClause != NULL)
        {
            *outConflictClause = conflict->literals;
        }
    }
    freeSolver(&solver);
    return result;
}

int8_t sat_getBackboneValue(SAT_Problem* problem, bool* solution, uint32_t varIndex)
{
    if(varIndex >= problem->numVars)
    {
        return 127;
    }
    SAT_Literal testedLit = (SAT_Literal)(varIndex + 1u);
    if(solution[varIndex] == false)
    {
        testedLit = -testedLit;
    }

    SAT_Literal testedNegated = -testedLit;
    bool        isSat         = sat_isSatisfiableWithAssumptions(problem, 1u, &testedNegated, NULL, NULL, NULL);
    if(isSat == false)
    {
        sat_addUnitClause(problem, testedLit);
        if(solution[varIndex] == false)
        {
            return -1;
        }
        else
        {
            return 1;
        }
    }
    return 0;
}

size_t sat_getNumClauses(const SAT_Problem* problem)
{
    return problem->numClauses;
}

uint32_t sat_getNumVariables(const SAT_Problem* problem)
{
    return problem->numVars;
}
