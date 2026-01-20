# Overview
Pioneer is a free UCI chess engine written from scratch in C++. It is heavily inspired by Sebastian Lague's chess engine videos and Stockfish.

# Features
## Search
* Alpha-Beta
* PVS Search
* Aspiration Windows
* Iterative Deepening
* Quiescence Search
* Internal Iterative Deepening
### Pruning
* Futility Pruning
* Null Move Pruning
* Razoring
* Reverse Futility Pruning
* Delta Pruning
* Transposition Table
### Extensions/Reductions
* Late Move Reductions
* Internal Iterative Reductions
* Check Extensions
### Move Ordering
* Killer Moves
* Move History
* Capture History
* Continuation History
* Counter Moves
* MVV LVA
* Hash Move
## Evaluation
* Material Score
* Piece Square Tables
* Passed Pawns
* Isolated Pawns
* Doubled Pawns
* Pawn Shield
* Bishop Pair
* Attacked King-Adjacent Squares

As of now, my evaluation is lacking the most. I'm working on replacing my evaluation with an NNUE :D
