# Cluedo Solver (Ultimate Detective mode)

![Language: C](https://img.shields.io/badge/Language-C-blue.svg)
[![Build](https://github.com/PHochmann/Cluedo/actions/workflows/build.yml/badge.svg)](https://github.com/PHochmann/Cluedo/actions/workflows/build.yml)
[![Test](https://github.com/PHochmann/Cluedo/actions/workflows/test.yml/badge.svg)](https://github.com/PHochmann/Cluedo/actions/workflows/test.yml)

Interactive Cluedo (Clue) assistant written in C23.

This project serves as an exercise to implement my own SAT solver.
It models game knowledge as an CNF, solves it with a custom CDCL SAT solver, and recommends the next move to reduce uncertainty about the envelope.

```
┌───────────────┬─────────┐
│               │ A  B  C │
├═══════════════┼═════════┤
│ Mustard       │ Y  X  X │
│ Plum          │ X  X  ? │
│ Green         │ X       │
│ Peacock       │ X       │
│ Scarlett      │ X       │
│ White         │ X       │
├───────────────┼─────────┤
│ Candlestick   │ Y  X  X │
│ Dagger        │ Y  X  X │
│ Pipe          │ X  ?  ? │
│ Revolver      │ X       │
│ Rope          │         │
│ Wrench        │ Y  X  X │
├───────────────┼─────────┤
│ Kitchen       │ X  ?  ? │
│ Ballroom      │ Y  X  X │
│ Conservatory  │ X       │
│ Billiard Room │ X       │
│ Library       │ X       │
│ Study         │ X       │
│ Hall          │ X       │
│ Lounge        │ Y  X  X │
│ Dining Room   │ X       │
└───────────────┴─────────┘
```

Legend:
- `Y`: literal is true in all satisfying assignments
- `X`: literal is false in all satisfying assignments
- `?`: card appeared in a positively answered guess but is still SAT-undecided
- blank: currently undecided by SAT and did not appear in a guess yet

## Overview

This application implements a knowledge base of the rules of Cluedo and the information entered in each round, encoded as boolean formulas. A SAT solver is used to deduce who committed the murder, with what weapon, and in which room.
The next move is calculated to give as much information about the envelope content as possible.

The solver targets *Ultimate Detective* mode from the 2023 digital release, which differs from classic rules:
- All players publicly answer yes/no for each shown suspect/weapon/room triple
- A key token can be used to ask a targeted private card question
- Players move directly between rooms; reachable rooms depend on a 1-3 dice roll

## Solver and knowledge base design

Variables encode ownership: literal $x_{p,c}$ means player $p$ (including envelope as pseudo-player) owns card $c$.

Initial model constraints:
1. Each card is owned by exactly one player (including envelope)
2. Envelope contains exactly one suspect, one weapon, one room
3. Each player holds exactly $n$ cards

Per-turn updates:
1. Guess feedback adds clauses
2. Key-token questions add clauses
3. Failed accusations add clauses

SAT internals:
- CDCL core with learned clauses from 1st UIP conflict cuts
- Two-Watched-Literals for Boolean constraint propagation
- Cardinality constraints encoded with auxiliary variables (Sinz encoding)
- Temporary clauses for SAT calls under assumptions inside recommendation logic
- Naive variable ordering (no heuristics yet)

### Recommendation strategy

The recommender samples five candidate suspect/weapon/room triples and computes, for each candidate, the average number of possible envelope triples after all feedback outcomes (computationally expensive).
The candidate with the lowest average remaining envelope possibilities is recommended.

## Literature

* [Conflict-driven clause learning (CDCL) SAT solvers (Aalto University, 2020)](https://users.aalto.fi/~tjunttil/2020-DP-AUT/notes-sat/cdcl.html)
    * Explanation of trail data structure
    * Very good illustration of *cuts*, including the 1st UIP-cut
* [Handbook of Satisfiability, Chapter 4: Conflict-Driven Clause Learning SAT Solvers (Princeton, 2008)](https://www.cs.princeton.edu/~zkincaid/courses/fall18/readings/SATHandbook-CDCL.pdf)
    * Contains algorithmic explanation to compute cuts
* [Towards an Optimal CNF Encoding of Boolean Cardinality Constraints (Sinz, 2005)](https://www.carstensinz.de/papers/CP-2005.pdf)
    * Presents an efficient technique to encode cardinality constraints
    * Used to encode that player has exactly $n$ cards
    * Without this, settings with fewer players (i.e. more cards per player) would not be possible because the number of clauses is exponential in terms of $n$ when encoded naively.

## Project Structure

- **src/**:
  - `main.c` - Game loop
  - `sat.c` - SAT solver implementation
  - `kb.c` - Encodes rules and information as boolean formulas in CNF
  - `recommender.c` - Next move recommendation
  - `sheet.c` - Prints the sheet
  - `cards.c` - Utility for card names
  - `parsing.c` - Utility for parsing with readline
  - **util/** - Table printing (older code of mine, currently not well-maintained)

## How to use it

Simply invoke:
```bash
make
```
Generates executable at `./bin/release/Cluedo`. Alternative targets are: `debug`, `format`, `clean`, `check`, `install-hooks`

Requirements:
- C compiler with C23 support
- GNU readline (`libreadline-dev`)
- Make

## Limitations

* Only *Ultimate Detective* mode is supported.
* The SAT solver currently uses no variable-ordering heuristics.
* The SAT solver currently does not delete learned clauses which slows it down on large problems

## License

This project is released into the public domain. You are free to use, modify, and distribute it without restriction.
