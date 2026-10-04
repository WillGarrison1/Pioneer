#ifndef SEARCH_PARAMS_H
#define SEARCH_PARAMS_H

#include "types.h"

constexpr int LMR_INDEX = 1;
constexpr int LMR_DEPTH = 2;
constexpr float LMR_DIVISOR = 2.25;
constexpr float LMR_OFFSET = 1.0f;

constexpr Score ASPIRATION_STARTING_DELTA = 30;
constexpr float ASPIRATION_MULTIPLIER = 1.5f;

constexpr int IIR_DEPTH = 3;

constexpr int FUTILITY_DEPTH = 4;
constexpr float FUTILITY_MULTI = 120;
constexpr float FUTILITY_OFFSET = 80;

constexpr float REVERSE_FUTILITY_MULTI = 120;
constexpr float REVERSE_FUTILITY_MAX_DEPTH = 8;

constexpr float RAZORING_OFFSET = 300;
constexpr float RAZORING_MULTI = 100;
constexpr int RAZORING_DEPTH = 3;

constexpr int NULL_DEPTH = 3;
constexpr int NULL_MOVE_VERIFY_DEPTH = 12;
constexpr int NULL_MOVE_DEPTH_OFFSET = 1;
constexpr float NULL_MOVE_DEPTH_MULTI = 2.0f / 3.0f;

constexpr float LMP_MULTI = 2;
constexpr float LMP_OFFSET = 3;

constexpr float DELTA = 200;

constexpr int CAPTURE_BONUS = 10000;
constexpr int PROMOTION_BONUS = 15000;
constexpr int ATTACKED_PENALTY = -10;
constexpr int PV_BONUS = 31000;
constexpr int KILLER_MOVE_BONUS = 20000;
constexpr int COUNTERMOVE_BONUS = 2000;
constexpr int MAX_HISTORY = 7500;
constexpr int MAX_CAPTURE_HISTORY = 2500;

#endif