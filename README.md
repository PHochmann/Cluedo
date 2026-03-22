# Cluedo Solver

A C-based interactive solver for the board game Cluedo (Clue). This project uses SAT solving and a knowledge base to help players deduce the solution through logical reasoning.

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

**Note:** This solver is designed for the Ultimate Detective variant of Cluedo, not the standard Cluedo rules.

### Features

- **Interactive Gameplay**: Uses readline for an interactive command-line interface
- **SAT Solver**: Custom implementation of a CDCL SAT solver for constraint-based deduction
- **Knowledge Base**: Maintains game state and logical constraints
- **Move Recommender**: Suggests strategic moves based on deduced information (Todo)
- **Sheet as Table**: Prints known cards as a table to the terminal

## Building

### Release Build
```bash
make
```
Generates executable at `./bin/release/Cluedo`

### Requirements

- C compiler with C99 support
- GNU readline (`libreadline-dev`)
- Make

## Project Structure

- **src/**:
  - `main.c` - Game loop
  - `sat.c/h` - SAT solver implementation (CDCL, 2WL)
  - `kb.c/h` - Encodes rules and information as boolean formulas in CNF
  - `recommender.c/h` - Move recommendation

## License

This project is released into the public domain. You are free to use, modify, and distribute it without restriction.
