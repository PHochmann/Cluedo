# Cluedo Solver (Ultimate Detective mode)

![Language: C](https://img.shields.io/badge/Language-C-blue.svg) [![Build](https://github.com/PHochmann/Cluedo/actions/workflows/build.yml/badge.svg)](https://github.com/PHochmann/Cluedo/actions/workflows/build.yml)

Interactive solver for the board game Cluedo (or Clue) written in C. This project uses SAT solving and a knowledge base to help players deduce the solution through logical reasoning.

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

## Overview

This application implements a knowledge base of the rules of Cluedo and the information entered in each round, encoded as boolean formulas. A SAT solver is used to deduce who committed the murder, with what weapon, and in which room.
The next move is calculated to give as much information about the envelope content as possible.

This solver is designed for the *Ultimate Detective* mode of Cluedo from the 2023 digital release. It differs in the following aspects from the classic rules:
* All players publicly give feedback (yes or no) to the shown triple of cards
* A player can use a token (key) to show a card of their choice to a player. The player answers privately if they have the card, or not. The token is then given to the player that was asked. Each player has one token at start.
* Players don't move on a grid but are always in a room. A dice roll between 1 and 3 decides how many adjacent rooms are reachable.

### SAT knowledge base and solver

The project serves as an exercise to implement my own SAT solver.
I chose the regular CDCL structure for which there exists much learning material (see *Literature* below).
The SAT solver maintains a formula in CNF and learns conflict-clauses of the 1st UIP cut.
It uses Two-Watched-Literals as a data structure for BCP.
Variable ordering is naive, without any heuristics.

Variables in the formula encode card ownership: Literal $x_{p,c}$ encodes that Player $p$ (including envelope as a pseudo-player) has card $c$ ($\neg x_{p,c}$ that they do not have $c$).
Auxiliary variables are introduced through cardinality constraints of player cards [(Sinz 2005)](https://www.carstensinz.de/papers/CP-2005.pdf).

#### How it works

1. Initial clauses are added to the CNF formula
    * Each card belongs to exactly one player (including envelope)
    * Envelope contains exactly one suspect, weapon, room
    * Each player has exactly $n$ cards
2. Each turn adds new clauses to the formula
    * Yes/no of regular guess yields a clause
    * Key guess yields a clause
    * Failed accusation yields a clause
3. The sheet gets updated
    * Sheet is for display only
    * Computes the backbone-value of all card-ownership-literals: True in all satisfying assignments, False in all satisfying assignments or undetermined
4. Next move is recommended
    * Five suspect/weapon/room-triples are chosen by a heuristic
    * Average number of possible envelope-triples after all possible feedback-combinations are computed (computationally very expensive)
    * Candidate with least number of average envelope-triples is chosen
    * SAT solver supports temporary clauses to test assumptions

#### Literature

* [Conflict-driven clause learning (CDCL) SAT solvers (Aalto University, 2020)](https://users.aalto.fi/~tjunttil/2020-DP-AUT/notes-sat/cdcl.html)
    * Explanation of trail data structure
    * Very good illustration of *cuts*, including the 1st UIP-cut
* [Handbook of Satisfiability, Chapter 4: Conflict-Driven Clause Learning SAT Solvers (Princeton, 2008)](https://www.cs.princeton.edu/~zkincaid/courses/fall18/readings/SATHandbook-CDCL.pdf)
    * Contains algorithmic explanation to compute cuts
* [Towards an Optimal CNF Encoding of Boolean Cardinality Constraints (Sinz, 2005)](https://www.carstensinz.de/papers/CP-2005.pdf)
    * Presents an efficient technique to encode cardinality constraints
    * Used to encode that player has exactly $n$ cards
    * Without this, settings with fewer players (i.e. more cards per player) would not be possible because the number of clauses is exponential in terms of $n$ when encoded naively.

### Project Structure

- **src/**:
  - `main.c` - Game loop
  - `sat.c` - SAT solver implementation
  - `kb.c` - Encodes rules and information as boolean formulas in CNF
  - `recommender.c` - Move recommendation
  - `sheet.c` - Prints the sheet containing certain, possible or impossible cards
  - `cards.c` - Utility for card names
  - `parsing.c` - Utility to for parsing with readline
  - **util/** - Table printing (older code of mine, currently not well-maintained)

## How to use it

Simply invoke:
```bash
make
```
Generates executable at `./bin/release/Cluedo`. Alternative targets are: `debug`, `format`, `clean`, `install-hooks`

Requirements:
- C compiler with C23 support
- GNU readline (`libreadline-dev`)
- Make

## Limitations

* Only Ultimate Detective mode is supported.
    * Todo: Add classic mode
* The SAT solver currently uses no variable-ordering heuristics.

## License

This project is released into the public domain. You are free to use, modify, and distribute it without restriction.
