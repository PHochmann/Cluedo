/**
 * @file main.c
 * SAT solver test program that reads DIMACS CNF format from stdin and solves it.
 */


//***************************************************************************//
//************************** INCLUDES ***************************************//
//***************************************************************************//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sat.h"

//***************************************************************************//
//**************************  PRIVATE DEFINES *******************************//
//***************************************************************************//

#define BUFFER_SIZE             1024
#define INITIAL_CLAUSE_CAPACITY 100

//***************************************************************************//
//************************** PRIVATE TYPEDEFS *******************************//
//***************************************************************************//

/**
 * @brief Result of reading a line from stdin.
 */
typedef enum
{
    READLINE_OK,         /**< A valid line was successfully read. */
    READLINE_COMMENT,    /**< A comment line ('c ...') was read. */
    READLINE_TERMINATOR, /**< A '%%' terminator line was encountered. */
    READLINE_EOF         /**< End of file or read error. */
} ReadLineResult;

/**
 * @brief Result of parsing a DIMACS CNF problem from stdin.
 */
typedef enum
{
    PARSE_SUCCESS, /**< A problem was parsed successfully. */
    PARSE_EOF,     /**< No more problems; EOF reached before a problem header. */
    PARSE_ERROR    /**< A parsing error occurred. */
} ParseResult;

/**
 * @brief Output data produced by parsing one DIMACS CNF problem.
 */
typedef struct
{
    bool         expectedSatisfiable; /**< True if the declared expected result is SAT, false if UNSAT. */
    SAT_Literal* backboneLiterals;    /**< Heap-allocated array of backbone literals; caller must free. */
    int          numBackboneLiterals; /**< Number of entries in backboneLiterals. */
} DimacsParseOutput;

//***************************************************************************//
//************************** PRIVATE FUNCTION DECLARATIONS ******************//
//***************************************************************************//

/**
 * @brief Reads and parses one DIMACS CNF problem from stdin.
 *
 * The last comment line before the problem header must contain either
 * "sat" or "unsat" to declare the expected result.
 * Stops reading clauses when the declared clause count is reached, or early
 * if a '%%' terminator or EOF is encountered. The '%%' terminator is optional;
 * if absent, reading stops naturally after the last clause.
 *
 * @param problem  SAT_Problem to populate with clauses.
 * @param out      Output data produced by parsing; caller must free out->backboneLiterals.
 * @return PARSE_SUCCESS if a problem was parsed, PARSE_EOF if stdin is
 *         exhausted before a problem header, or PARSE_ERROR on failure.
 */
static ParseResult parseDimacsCnf(SAT_Problem* problem, DimacsParseOutput* out);

//***************************************************************************//
//************************** PRIVATE FUNCTION DEFINITIONS *******************//
//***************************************************************************//

/**
 * @brief Counts the number of spaces in a string.
 *
 * @param str The string to count spaces in.
 * @return Number of space characters found.
 */
static int countSpaces(const char* str)
{
    int count = 0;
    for(int i = 0; str[i] != '\0'; i++)
    {
        if(str[i] == ' ')
        {
            count++;
        }
    }
    return count;
}

/**
 * @brief Reads a line from input, skipping comments and empty lines.
 *
 * @param buffer Buffer to store the line.
 * @param size Size of the buffer.
 * @return READLINE_OK if a valid line was read, READLINE_TERMINATOR if a '%%'
 *         terminator was encountered, or READLINE_EOF on end of file.
 */
static ReadLineResult readValidLine(char* buffer, size_t size)
{
    while(fgets(buffer, (int)size, stdin) != NULL)
    {
        if(buffer[0] == '%')
        {
            return READLINE_TERMINATOR;
        }

        if(buffer[0] == 'c')
        {
            return READLINE_COMMENT;
        }

        // Skip empty lines
        if(buffer[0] != '\n' && buffer[0] != '\r')
        {
            return READLINE_OK;
        }
    }
    return READLINE_EOF;
}

static ParseResult parseDimacsCnf(SAT_Problem* problem, DimacsParseOutput* out)
{
    char           buffer[BUFFER_SIZE];
    char           lastComment[BUFFER_SIZE];
    int            numVars             = 0;
    int            numClauses          = 0;
    int            clausesParsed       = 0;
    SAT_Literal*   backboneLiterals    = NULL;
    int            numBackboneLiterals = 0;
    int            backboneCapacity    = 0;
    ReadLineResult lineResult;

    lastComment[0]           = '\0';
    out->backboneLiterals    = NULL;
    out->numBackboneLiterals = 0;

    // Read problem line: "p cnf <num_vars> <num_clauses>", skipping any '%' terminators left over from the previous problem.
    do
    {
        lineResult = readValidLine(buffer, sizeof(buffer));
        if(lineResult == READLINE_COMMENT)
        {
            strncpy(lastComment, buffer, sizeof(lastComment) - 1);
            lastComment[sizeof(lastComment) - 1] = '\0';

            int lit = 0;
            if(sscanf(buffer, "c backbone %d", &lit) == 1)
            {
                if(numBackboneLiterals == backboneCapacity)
                {
                    backboneCapacity = (backboneCapacity == 0) ? 16 : (backboneCapacity * 2);
                    backboneLiterals = realloc(backboneLiterals, ((size_t)backboneCapacity * sizeof(SAT_Literal)));
                }
                backboneLiterals[numBackboneLiterals] = (SAT_Literal)lit;
                numBackboneLiterals++;
            }
        }
    } while((lineResult == READLINE_TERMINATOR) || (lineResult == READLINE_COMMENT));

    if(lineResult == READLINE_EOF)
    {
        free(backboneLiterals);
        return PARSE_EOF;
    }

    if(strstr(lastComment, "unsat") != NULL)
    {
        out->expectedSatisfiable = false;
    }
    else if(strstr(lastComment, "sat") != NULL)
    {
        out->expectedSatisfiable = true;
    }
    else
    {
        fprintf(stderr, "Error: expected result comment ('sat' or 'unsat') missing before problem header\n");
        free(backboneLiterals);
        return PARSE_ERROR;
    }

    if(sscanf(buffer, "p cnf %d %d", &numVars, &numClauses) != 2)
    {
        fprintf(stderr, "Error: invalid problem line format\n");
        free(backboneLiterals);
        return PARSE_ERROR;
    }

    if((numVars < 0) || (numClauses < 0))
    {
        fprintf(stderr, "Error: invalid number of variables or clauses\n");
        free(backboneLiterals);
        return PARSE_ERROR;
    }

    // Read clauses
    while(clausesParsed < numClauses)
    {
        lineResult = readValidLine(buffer, sizeof(buffer));
        if((lineResult == READLINE_EOF) || (lineResult == READLINE_TERMINATOR))
        {
            break;
        }

        if(lineResult == READLINE_COMMENT)
        {
            continue;
        }

        int          maxLiterals = countSpaces(buffer);
        SAT_Literal* literals    = malloc(sizeof(SAT_Literal) * (size_t)maxLiterals);
        int          numLiterals = 0;

        const char* ptr       = buffer;
        int         literal   = 0;
        int         bytesRead = 0;

        while(sscanf(ptr, "%d%n", &literal, &bytesRead) == 1)
        {
            if(literal == 0)
            {
                // End of clause
                break;
            }

            literals[numLiterals] = literal;
            numLiterals++;

            // Advance pointer by the number of bytes read
            ptr = ptr + bytesRead;
        }

        sat_vAddClause(problem, (uint32_t)numLiterals, literals);
        free(literals);
        clausesParsed++;
    }

    if(clausesParsed != numClauses)
    {
        fprintf(stderr, "Warning: expected %d clauses but parsed %d\n", numClauses, clausesParsed);
    }

    out->backboneLiterals    = backboneLiterals;
    out->numBackboneLiterals = numBackboneLiterals;

    return PARSE_SUCCESS;
}

//***************************************************************************//
//************************** PUBLIC FUNCTION DEFINITIONS ********************//
//***************************************************************************//

int main(void)
{
    int  counter   = 1;
    bool allPassed = true;
    while(true)
    {
        SAT_Problem*      problem     = sat_initProblem();
        DimacsParseOutput parseOutput = { false, NULL, 0 };
        ParseResult       parseResult = parseDimacsCnf(problem, &parseOutput);

        if(parseResult == PARSE_EOF)
        {
            sat_freeProblem(problem);
            break;
        }

        if(parseResult == PARSE_ERROR)
        {
            fprintf(stderr, "Error: failed to parse DIMACS CNF\n");
            sat_freeProblem(problem);
            return 1;
        }

        printf("[%d] Num. variables: %zu, Num. clauses: %zu: ", counter, sat_getNumVariables(problem), sat_getNumClauses(problem));

        // Solve the SAT problem
        const SAT_Result* result      = NULL;
        bool              satisfiable = sat_isSatisfiable(problem, &result);

        // Check backbone literals
        bool backbonePassed = true;
        if((satisfiable == true) && (parseOutput.numBackboneLiterals > 0))
        {
            for(int i = 0; i < parseOutput.numBackboneLiterals; i++)
            {
                SAT_Literal    lit      = parseOutput.backboneLiterals[i];
                uint32_t       varIndex = (uint32_t)((lit > 0) ? (lit - 1) : ((-lit) - 1));
                SAT_Assignment expected = (lit > 0) ? SAT_TRUE : SAT_FALSE;
                SAT_Assignment actual   = sat_getBackboneValue(problem, result->solution, varIndex);
                if(actual != expected)
                {
                    backbonePassed = false;
                }
            }
        }

        // Print result
        bool satPassed = (satisfiable == parseOutput.expectedSatisfiable);
        if((satPassed == true) && (backbonePassed == true))
        {
            printf("PASS\n");
        }
        else
        {
            printf("FAIL");
            if(satPassed == false)
            {
                printf(" (expected %s, got %s)", (parseOutput.expectedSatisfiable == true) ? "sat" : "unsat", (satisfiable == true) ? "sat" : "unsat");
            }
            if(backbonePassed == false)
            {
                printf(" (backbone mismatch)");
            }
            printf("\n");
            allPassed = false;
        }

        // Cleanup
        if(result != NULL)
        {
            sat_freeResult(result);
        }
        free(parseOutput.backboneLiterals);
        sat_freeProblem(problem);
        counter++;
    }

    return allPassed ? 0 : 1;
}
