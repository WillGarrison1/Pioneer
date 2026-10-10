import subprocess, random, math, os, copy, shutil, json
from dataclasses import dataclass, field
from pathlib import Path


@dataclass
class Param:
    name: str
    value: float
    min_val: float
    max_val: float
    c_end: float
    r_end: float = 0.002
    is_int: bool = False


PARAMS = [
    Param("LMR_INDEX", 1, 1, 10, 2.5, is_int=True),
    Param("LMR_DEPTH", 2, 1, 10, 2.5, is_int=True),
    Param("IIR_DEPTH", 3, 1, 10, 2.5, is_int=True),
    Param("FUTILITY_DEPTH", 4, 1, 10, 2.5, is_int=True),
    Param("RAZORING_DEPTH", 3, 1, 10, 2.5, is_int=True),
    Param("NULL_DEPTH", 3, 2, 10, 2.5, is_int=True),
    Param("NULL_MOVE_DEPTH_OFFSET", 1, 0, 7, 2.5, is_int=True),
    Param("REVERSE_FUTILITY_MAX_DEPTH", 8, 2, 20, 2.5, is_int=True),
    Param("NULL_MOVE_VERIFY_DEPTH", 12, 2, 20, 2.5, is_int=True),
    Param("LMR_DIVISOR", 2.25, 0.5, 5, 1),
    Param("LMR_OFFSET", 1, 0.1, 5.0, 1),
    Param("ASPIRATION_STARTING_DELTA", 30, 10, 100, 30),
    Param("ASPIRATION_MULTIPLIER", 1.5, 1.1, 3, 0.3),
    Param("FUTILITY_MULTI", 120, 30, 300, 20),
    Param("FUTILITY_OFFSET", 80, 10, 200, 50),
    Param("REVERSE_FUTILITY_MULTI", 120, 20, 200, 50),
    Param("RAZORING_OFFSET", 300, 100, 500, 125),
    Param("RAZORING_MULTI", 100, 20, 200, 50),
    Param("NULL_MOVE_DEPTH_MULTI", 2.0 / 3.0, 1.0 / 3.0, 5.0 / 6.0, 0.33),
    Param("LMP_MULTI", 2, 1.25, 5, 1.25),
    Param("LMP_OFFSET", 3, 2, 9, 2),
    Param("DELTA", 200, 50, 500, 100),
    Param("CAPTURE_BONUS", 10000, 0, (1 << 15) - 1, 5000, is_int=True),
    Param("PROMOTION_BONUS", 15000, 0, (1 << 15) - 1, 5000, is_int=True),
    Param("ATTACKED_PENALTY", -10, -(1 << 15), 0, 5000, is_int=True),
    Param("PV_BONUS", 31000, 0, (1 << 15) - 1, 1, is_int=True),
    Param("KILLER_MOVE_BONUS", 20000, 0, (1 << 15) - 1, 5000, is_int=True),
    Param("COUNTERMOVE_BONUS", 2000, 0, (1 << 15) - 1, 5000, is_int=True),
    Param("MAX_HISTORY", 7500, 0, (1 << 15) - 1, 5000, is_int=True),
    Param("MAX_CAPTURE_HISTORY", 2500, 0, (1 << 15) - 1, 5000, is_int=True),
]

TOTAL_ITERATIONS = 1000
GAMES_PER_ITER = 30
CONCURRENCY = 10
TC = "10+0.1"
BOOK = "book.epd"
ENGINE_SRC = Path(".")
RESULTS_FILE = "spsa_results.json"


A_RATIO = 0.1
ALPHA = 0.602
GAMMA = 0.101


def spsa_a(p: Param) -> float:
    # Compute 'a' so that the first step moves value by ~r_end * (max-min)
    return (
        p.r_end
        * (max(1, A_RATIO * TOTAL_ITERATIONS + TOTAL_ITERATIONS)) ** ALPHA
        * (p.max_val - p.min_val)
    )


def run_match(params_plus, params_minus) -> float:
    """Returns score from plus-engine's perspective: 1=win, 0.5=draw, 0=loss."""
    build_engine(params_plus, "engine_plus")
    build_engine(params_minus, "engine_minus")

    try:
        # fmt: off
        result = subprocess.run([
            "fastchess",
            "-engine", "cmd=./engine_plus",  "name=plus",
            "-engine", "cmd=./engine_minus", "name=minus",
            "-each", f"proto=uci", f"tc={TC}",
            "-openings", f"file={BOOK}", "format=epd", "order=random",
            "-repeat", "-games", str(GAMES_PER_ITER),
            "-rounds", "1",
            "-resign", "movecount=3", "score=400",
            "-draw", "movenumber=40", "movecount=8", "score=10",
            "-concurrency", str(CONCURRENCY),
            "-output", "format=cutechess",
        ], capture_output=True, text=True, timeout=900)
        # fmt: on
    except subprocess.TimeoutExpired:
        return 0.5

    # Parse score from output
    for line in result.stdout.splitlines()[::-1]:
        if "score of plus" in line.lower():
            # cutechess format: "Score of plus vs minus: W - D - L [winrate]"
            rate = float(line.split("[")[1].split("]")[0])
            print(f"Match finished (plus vs minus) - {rate:.3f}")
            return rate
    return 0.5  # no result found


def save_results(iteration: int, params: list[Param]):
    data = {"iteration": iteration, "params": {p.name: p.value for p in params}}
    with open(RESULTS_FILE, "w") as f:
        json.dump(data, f, indent=2)
    print(f"  Saved: { {p.name: round(p.value,1) for p in params} }")


def build_engine(params: list[Param], output_name: str):
    with open("src/tuning_params.h", "w") as f:
        f.write("#pragma once\n\n")
        for p in params:
            if p.is_int:
                f.write(f"constexpr int {p.name} = {round(p.value)};\n")
            else:
                f.write(f"constexpr float {p.name} = {p.value:.4f};\n")

    subprocess.run(
        [
            "cmake",
            "--preset",
            "Release",
            "-DCMAKE_BUILD_TYPE=Release",
            "-DCMAKE_CXX_FLAGS=-DTUNING",
        ],
        check=True,
        capture_output=True,
    )
    subprocess.run(
        ["cmake", "--build", "out/build/Release", "--config", "Release"],
        check=True,
        capture_output=True,
    )
    shutil.copy("out/build/Release/PioneerV4.exe", output_name)


def main():
    params = copy.deepcopy(PARAMS)

    # Load checkpoint if exists
    start_iter = 1
    if Path(RESULTS_FILE).exists():
        with open(RESULTS_FILE) as f:
            data = json.load(f)
        start_iter = data["iteration"] + 1
        for p in params:
            if p.name in data["params"]:
                p.value = data["params"][p.name]
        print(f"Resuming from iteration {start_iter}")

    for k in range(start_iter, TOTAL_ITERATIONS + 1):
        # Compute step sizes for this iteration
        ck = [p.c_end / k**GAMMA for p in params]
        ak = [spsa_a(p) / (A_RATIO * TOTAL_ITERATIONS + k) ** ALPHA for p in params]

        # Random perturbation direction (Bernoulli ±1)
        delta = [random.choice([-1, 1]) for _ in params]

        # Build perturbed copies
        params_plus = copy.deepcopy(params)
        params_minus = copy.deepcopy(params)
        for i, p in enumerate(params):
            params_plus[i].value = min(
                p.max_val, max(p.min_val, p.value + ck[i] * delta[i])
            )
            params_minus[i].value = min(
                p.max_val, max(p.min_val, p.value - ck[i] * delta[i])
            )

        import hashlib

        def _h(p):
            return hashlib.md5(Path(p).read_bytes()).hexdigest()[:8]

        build_engine(params_plus, "engine_plus")
        build_engine(params_minus, "engine_minus")
        hp, hm = _h("engine_plus"), _h("engine_minus")
        print(
            f"  plus={hp} minus={hm} {'! IDENTICAL — build not picking up params!' if hp==hm else 'ok'}"
        )

        # Run match
        print(f"Running match: {k}")
        score = run_match(params_plus, params_minus)
        gradient_estimate = (score - 0.5) / 0.5  # normalize to [-1, 1]

        # Update parameters
        for i, p in enumerate(params):
            gradient_i = gradient_estimate / (ck[i] * delta[i])
            p.value = min(p.max_val, max(p.min_val, p.value + ak[i] * gradient_i))

        if k % 10 == 0:
            print(f"\nIteration {k}/{TOTAL_ITERATIONS}")
            save_results(k, params)

    print("\n=== Final parameters ===")
    for p in params:
        print(f"  {p.name} = {round(p.value, 1)}")
    save_results(TOTAL_ITERATIONS, params)


if __name__ == "__main__":
    main()
