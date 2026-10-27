# C++ Chess Engine

A UCI-compatible chess engine written in C++. This repository contains the engine's board representation, move generation, attack detection, position hashing, evaluation, search, transposition table, and UCI interface.

> **Implementation note:** This guide documents the standard UCI workflow and the build command currently used for this project. UCI commands such as `uci`, `isready`, `position`, and `go` are part of the protocol. Options such as thread count and pondering must also be implemented by the engine before they will work; see [Multithreading](#multithreading) and [Pondering](#pondering).

## Contents

- [Requirements](#requirements)
- [Compile the engine](#compile-the-engine)
- [Run the engine](#run-the-engine)
- [Use the UCI protocol manually](#use-the-uci-protocol-manually)
- [UCI command reference](#uci-command-reference)
- [Search limits](#search-limits)
- [Multithreading](#multithreading)
- [Pondering](#pondering)
- [Connect to a chess GUI](#connect-to-a-chess-gui)
- [Project files](#project-files)
- [Troubleshooting](#troubleshooting)

## Requirements

- A C++ compiler with support for the C++ standard used by the source code.
- `g++` (for the build command below).
- A terminal such as PowerShell, Command Prompt, Windows Terminal, or a Linux/macOS shell.
- Optionally, a UCI-compatible chess GUI such as Cute Chess or Arena.

The compile command below uses `-O3` for optimization. It does not explicitly select a C++ language standard or enable platform-specific CPU instructions.

## Compile the engine

The source files are currently expected to be inside the `bitboards` directory. Open a terminal **in that directory**, then run:

```bash
g++ -O3 attacks.cpp bitboards.cpp board.cpp magic.cpp main.cpp makeMove.cpp movegen.cpp perft.cpp zobrist.cpp uci.cpp search.cpp evaluation.cpp see.cpp TT.cpp -o chess
```

This compiles the listed source files and produces an executable named `chess` (or `chess.exe` on Windows).

### Windows PowerShell example

If your repository is at `D:\vscode\chess-engine\bitboards`, run:

```powershell
cd D:\vscode\chess-engine\bitboards
g++ -O3 attacks.cpp bitboards.cpp board.cpp magic.cpp main.cpp makeMove.cpp movegen.cpp perft.cpp zobrist.cpp uci.cpp search.cpp evaluation.cpp see.cpp TT.cpp -o chess.exe
```

Adjust the path to match your computer. If `g++` is not recognized, install a GCC toolchain (for example, MinGW-w64) and ensure its `bin` directory is on your `PATH`.

### Run after compiling

**PowerShell:**

```powershell
.\chess.exe
```

**Linux/macOS shell:**

```bash
./chess
```

The engine communicates through standard input and standard output using UCI. When started this way, it will wait for protocol commands; it is not necessarily a graphical chess application.

## Use the UCI protocol manually

UCI stands for **Universal Chess Interface**. It is a text protocol that allows a chess GUI or another program to control the engine.

Start the executable, then type one command per line. A typical initial handshake is:

```text
uci
```

A UCI engine should respond with its engine identification, any supported `option` declarations, and finally:

```text
uciok
```

Then check readiness:

```text
isready
```

Expected response:

```text
readyok
```

The exact engine name and options depend on the implementation.

### Example: search the starting position

```text
uci
isready
ucinewgame
position startpos
go depth 8
```

The engine should print search information (usually lines beginning with `info`) and eventually a best move, for example:

```text
bestmove e2e4
```

The example move above is illustrative only; the actual move depends on the engine's search.

To stop a search early, send:

```text
stop
```

The engine should finish the current search cleanly and return a `bestmove` response.

To exit:

```text
quit
```

**Important:** UCI is a line-oriented protocol, not a conversational shell. Do not expect the engine to understand arbitrary sentences. Some engines also print diagnostic messages to the terminal; protocol output intended for a GUI should remain on standard output, while debug logging should ideally go to standard error.

## UCI command reference

| Command | Purpose |
|---|---|
| `uci` | Begin UCI initialization. The engine identifies itself, lists supported options, and replies with `uciok`. |
| `isready` | Ask whether the engine is ready for commands. It replies `readyok` when ready. |
| `ucinewgame` | Tell the engine a new game is starting so it can clear game-specific search state as appropriate. |
| `position startpos` | Set the board to the standard starting position. |
| `position startpos moves e2e4 e7e5` | Set the starting position and apply the listed moves in UCI coordinate notation. |
| `position fen <FEN>` | Set the board to a specific position described by Forsyth–Edwards Notation (FEN). |
| `position fen <FEN> moves <moves>` | Set a FEN position and then apply the listed moves. |
| `go depth 10` | Search to a maximum depth of 10 plies. |
| `go movetime 1000` | Search for approximately 1000 milliseconds, subject to implementation and scheduling. |
| `go nodes 100000` | Search up to a node limit of 100,000, if supported by the search implementation. |
| `go wtime 300000 btime 300000 winc 2000 binc 2000` | Search using clock times and increments, as sent by a GUI. |
| `stop` | Stop an active search and return a best move. |
| `ponderhit` | Tell the engine that the opponent made the predicted move while the engine was pondering. |
| `setoption name <name> value <value>` | Set a declared engine option, such as a supported thread-count option. |
| `quit` | Shut down the engine. |

Commands must follow UCI syntax. In particular, `position` is normally followed by `startpos` or `fen`, and `go` starts a search. Not every optional `go` limit or engine option is necessarily supported by every implementation.

### Move notation

UCI moves use coordinate notation:

- `e2e4`: move the piece from e2 to e4.
- `g1f3`: move the piece from g1 to f3.
- `e7e8q`: promote a pawn to a queen.
- `e7e8n`: promote a pawn to a knight.

Castling is represented as a king move, such as `e1g1` for White's kingside castle. The exact legality of a move is determined by the current position.

## Search limits

The `go` command can include one or more limits. Common examples:

```text
go depth 12
go movetime 2000
go nodes 500000
go wtime 300000 btime 300000 winc 2000 binc 2000
```

- **Depth** is measured in plies, where one ply is one side's move.
- **Movetime** is a time budget in milliseconds.
- **Nodes** limits the number of searched nodes.
- **Clock-based search** provides remaining time and increment for each side; a time-management routine decides how much time to spend.

When testing, use repeatable conditions (same position, limit, hardware, and build) to compare changes fairly. A deeper search does not guarantee a better move in every individual position, but it generally gives the engine more opportunity to examine variations.

## Multithreading

Multithreading lets an engine use more than one CPU thread to search positions. The UCI protocol does **not** automatically make an engine multithreaded: the engine must implement parallel search and expose an option if users are expected to configure the thread count.

A common convention is an option named `Threads`. If this engine declares that option in its response to `uci`, a GUI or terminal can set it using the exact option name shown by the engine, for example:

```text
setoption name Threads value 4
isready
```

Use a positive integer appropriate for your CPU. More threads do not always mean proportionally faster search: parallel-search overhead, shared transposition-table contention, and the search algorithm affect scaling.

### How to check whether it is supported

1. Start the engine and send `uci`.
2. Inspect the output for an option declaration such as `option name Threads type spin ...`.
3. If no thread option is listed, do not assume `setoption name Threads` will work.
4. Confirm that the implementation actually launches or uses worker threads; accepting an option alone is not proof of parallel search.

If the engine does not yet implement parallel search, it should be described as single-threaded rather than claiming multithreaded support.

## Pondering

**Pondering** means thinking during the opponent's turn. The engine predicts a likely opponent move and searches the resulting position before that move is actually played. If the prediction is correct, the engine can reuse some of the work and respond more quickly.

In UCI, a GUI can start a ponder search with a command shaped like:

```text
go ponder wtime 300000 btime 300000 winc 2000 binc 2000
```

The engine is expected to continue pondering until the GUI sends one of the relevant control commands:

- `ponderhit`: the predicted move was played. The engine should continue the search as a normal search under the applicable time controls.
- `stop`: the prediction was wrong or the GUI wants the search stopped. The engine should stop and return a valid `bestmove`.

The actual behavior depends on the engine's implementation and the GUI's configuration. A correct implementation needs to manage the ponder position, stop/continue transitions, time management, and search-thread synchronization safely.

### How to check whether pondering is supported

1. Check the engine's source code for handling of `go ponder` and `ponderhit`.
2. Verify that the search does not return immediately just because it is pondering.
3. Test both cases: the predicted move is played (`ponderhit`) and a different move is played (`stop`, followed by a new `position` and `go`).
4. Test through a UCI GUI with pondering enabled, and verify that the engine remains responsive to stop/quit commands.

Do not enable pondering in a GUI and assume it works merely because the GUI offers the setting. The engine must implement the protocol behavior correctly.

## Connect to a chess GUI

A UCI GUI can display the board, manage games and clocks, and send positions and search commands to the engine.

In most GUIs, the workflow is approximately:

1. Compile the engine and locate the resulting executable (`chess.exe` on Windows or `chess` on Linux/macOS).
2. Open the GUI's engine-management or engine-configuration section.
3. Add an external engine and select the executable.
4. Select the UCI protocol if the GUI asks.
5. Start a game or analysis session.
6. Configure options that the engine actually advertises, such as `Threads`, if available.

The exact menu names depend on the GUI. Cute Chess, for example, can be used to run engine-vs-engine matches as well as interactive testing.

## Project files

The current build command compiles these source files:

| File | Likely responsibility based on its name |
|---|---|
| `attacks.cpp` | Attack detection and attack-related routines |
| `bitboards.cpp` | Bitboard representation and operations |
| `board.cpp` | Board state and position handling |
| `magic.cpp` | Magic-bitboard attack lookup support |
| `main.cpp` | Program entry point |
| `makeMove.cpp` | Making and/or undoing moves |
| `movegen.cpp` | Legal or pseudo-legal move generation |
| `perft.cpp` | Perft move-generation testing |
| `zobrist.cpp` | Zobrist position hashing |
| `uci.cpp` | UCI command parsing and protocol responses |
| `search.cpp` | Search algorithm |
| `evaluation.cpp` | Position evaluation |
| `see.cpp` | Static exchange evaluation |
| `TT.cpp` | Transposition table |

These descriptions are inferred from filenames; consult each source file for its exact responsibilities.

## Troubleshooting

### `g++` is not recognized

Install a GCC toolchain and add its `bin` directory to your `PATH`. Restart the terminal and verify:

```bash
g++ --version
```

### `No such file or directory`

Run the compile command from the directory containing all the listed `.cpp` files, or update the command with the correct paths.

### Compilation errors

Check that all source files are present, that the compiler supports the C++ features used by the project, and that the command includes every required translation unit. If the code requires a particular standard, add the appropriate flag, such as `-std=c++17`, after confirming which standard the code needs.

### The GUI cannot communicate with the engine

- Confirm the executable path is correct.
- Start the engine manually and send `uci`; verify it eventually replies with `uciok`.
- Verify that protocol responses go to standard output and are not mixed with arbitrary logging.
- Ensure the engine handles `isready`, `position`, `go`, `stop`, and `quit` correctly.

### The engine ignores `Threads` or pondering

The option or behavior may not be implemented. Check the engine's `uci` output and the UCI command handling in the source before enabling it in a GUI.

---

## Development and testing

Useful development checks include:

- **Perft tests** to validate move generation and special moves.
- **UCI smoke tests** to verify the handshake and search lifecycle.
- **Tactical positions** to inspect search and evaluation behavior.
- **Engine matches** under repeatable time controls to measure playing strength.
- **Thread and ponder tests** to verify cancellation, synchronization, and protocol compliance when those features are implemented.

Contributions, bug reports, and reproducible test positions are welcome.
