# Overview
Pioneer is a free UCI chess engine written from scratch in C++. It is heavily inspired by Sebastian Lague's chess engine videos and Stockfish.

# Features
## Search
* Alpha-Beta
* PVS Search
* Aspiration Windows
* Iterative Deepening
* Quiescence Search
### Pruning
* Futility Pruning
* Null Move Pruning
* Razoring
* Reverse Futility Pruning
* Delta Pruning
* Transposition Table
* Late Move Pruning
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
* Piece Square Tables
## Evaluation
### HCE [Depricated]
* Material Score
* Piece Square Tables
* Passed Pawns
* Isolated Pawns
* Doubled Pawns
* Pawn Shield
* Bishop Pair
* Attacked King-Adjacent Squares

### NNUE
* Architecture;
  * (22528)x2 -> (1024+8)x2 -> (512)x2 -> 16 -> 32 -> 1
  * 8 subnets
  * 8 psqt bonuses
* SIMD accelerated
* Lazily Updated Accumulators
