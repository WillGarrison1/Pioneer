#!/usr/bin/env python3
"""
UCI smoke tests for Pioneer.

Drives the engine over a real pipe the way a GUI does -- send a command, then block reading
until the expected response arrives -- rather than dumping commands into stdin and hoping. That
distinction matters: piping a script into the engine closes stdin immediately, which tears the
engine down before an asynchronous search can report `bestmove`.

Usage:  python tools/uci_test.py [path-to-engine]
"""

import subprocess
import sys
import time
from pathlib import Path

DEFAULT_ENGINE = Path(__file__).resolve().parent.parent / "build" / "PioneerV4.exe"


class Engine:
    def __init__(self, path):
        self.proc = subprocess.Popen(
            [str(path)],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,
            text=True,
            bufsize=1,
        )

    def send(self, cmd):
        self.proc.stdin.write(cmd + "\n")
        self.proc.stdin.flush()

    def read_until(self, prefix, timeout=60.0):
        """Read lines until one starts with `prefix`. Returns (matched_line, all_lines)."""
        deadline = time.time() + timeout
        lines = []
        while time.time() < deadline:
            line = self.proc.stdout.readline()
            if not line:
                break
            line = line.strip()
            lines.append(line)
            if line.startswith(prefix):
                return line, lines
        return None, lines

    def quit(self):
        try:
            self.send("quit")
            self.proc.wait(timeout=5)
        except Exception:
            self.proc.kill()


RESULTS = []


def check(name, ok, detail=""):
    RESULTS.append((name, ok, detail))
    print(f"{'PASS' if ok else 'FAIL'}  {name}" + (f"  [{detail}]" if detail else ""))


def timed_go(eng, gocmd, timeout=60.0):
    start = time.time()
    eng.send(gocmd)
    line, lines = eng.read_until("bestmove", timeout=timeout)
    return (time.time() - start) * 1000.0, line, lines


def main():
    engine_path = Path(sys.argv[1]) if len(sys.argv) > 1 else DEFAULT_ENGINE
    if not engine_path.exists():
        print(f"engine not found: {engine_path}")
        return 1

    eng = Engine(engine_path)

    # --- handshake -------------------------------------------------------------------------
    eng.send("uci")
    line, lines = eng.read_until("uciok", timeout=30)
    check("uci -> uciok", line is not None)
    check("declares Hash option", any(l.startswith("option name Hash") for l in lines))

    eng.send("isready")
    line, _ = eng.read_until("readyok", timeout=30)
    check("isready -> readyok", line is not None)

    # --- setoption -------------------------------------------------------------------------
    eng.send("setoption name Hash value 128")
    eng.send("isready")
    line, _ = eng.read_until("readyok", timeout=30)
    check("setoption Hash 128 accepted", line is not None)

    # --- fixed depth -----------------------------------------------------------------------
    eng.send("position startpos")
    _, line, _ = timed_go(eng, "go depth 8")
    check("go depth 8 -> bestmove", line is not None, line or "")

    # --- time management -------------------------------------------------------------------
    # Increment must actually be spent. Deliberately uses a LARGE increment: with a realistic
    # 80ms increment the budget difference is ~13%, which is smaller than run-to-run timing
    # noise on a loaded or thermally-limited machine, and the comparison flaps. A 2s increment
    # makes the expected difference ~4x, which noise cannot manufacture or hide.
    eng.send("position startpos")
    inc_ms, line, _ = timed_go(eng, "go wtime 8000 btime 8000 winc 2000 binc 2000")
    check("large-increment TC -> bestmove", line is not None, f"{inc_ms:.0f} ms")

    eng.send("position startpos")
    sd_ms, line, _ = timed_go(eng, "go wtime 8000 btime 8000")
    check("sudden-death TC -> bestmove", line is not None, f"{sd_ms:.0f} ms")

    check(
        "increment is actually spent",
        inc_ms > sd_ms * 2,
        f"inc={inc_ms:.0f}ms vs sd={sd_ms:.0f}ms (want inc > 2x sd)",
    )

    # Hard limit: must never spend anywhere near the whole clock on one move.
    check("stays well under clock", inc_ms < 8000 * 0.45, f"{inc_ms:.0f} ms of 8000")

    eng.send("position startpos")
    mtg_ms, line, _ = timed_go(eng, "go wtime 30000 btime 30000 movestogo 10")
    check("movestogo -> bestmove", line is not None, f"{mtg_ms:.0f} ms")

    # movetime must be respected reasonably tightly.
    eng.send("position startpos")
    mt_ms, line, _ = timed_go(eng, "go movetime 1000")
    check("go movetime 1000 honoured", line is not None and 700 < mt_ms < 2500, f"{mt_ms:.0f} ms")

    # --- go infinite + stop ----------------------------------------------------------------
    eng.send("position startpos")
    eng.send("go infinite")
    time.sleep(1.5)
    start = time.time()
    eng.send("stop")
    line, _ = eng.read_until("bestmove", timeout=15)
    stop_ms = (time.time() - start) * 1000.0
    check("go infinite + stop -> bestmove", line is not None, f"stopped in {stop_ms:.0f} ms")

    # --- mate score reporting --------------------------------------------------------------
    # Back-rank mate in 1: Ra8#.
    eng.send("position fen 6k1/5ppp/8/8/8/8/8/R5K1 w - - 0 1")
    _, line, lines = timed_go(eng, "go depth 10")
    mate_lines = [l for l in lines if "score mate" in l]
    check("reports 'score mate' not raw cp", len(mate_lines) > 0,
          mate_lines[-1][:90] if mate_lines else "no mate score seen")

    eng.quit()

    failed = [n for n, ok, _ in RESULTS if not ok]
    print()
    print(f"{len(RESULTS) - len(failed)}/{len(RESULTS)} passed")
    if failed:
        print("failed: " + ", ".join(failed))
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
