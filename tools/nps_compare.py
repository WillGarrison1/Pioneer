#!/usr/bin/env python3
"""
Compare engine throughput between two builds, reliably.

Naive `bench` timing on a laptop is close to worthless. This machine (i3-1215U) is a hybrid
2 P-core + 4 E-core part: whether the search thread lands on a P-core or an E-core is roughly a
2x difference in nps, which swamps the 5-10% effects you are usually trying to measure. Observed
spreads of 450k-1000k nps on a *single unchanged binary* come from exactly this.

So this script:
  * pins the engine to one specific logical CPU (default 0, a P-core thread),
  * raises process priority to reduce preemption,
  * interleaves A/B runs so thermal drift affects both equally,
  * reports best-of-N (throughput noise is one-sided: interference only ever makes it slower,
    so the best sample is the closest to the machine's true capability -- the mean is not),
  * and verifies node counts match, since a throughput change must not alter search behaviour.

Usage:
    python tools/nps_compare.py build-pgo-plain build-pgo --depth 13 --rounds 6
    python tools/nps_compare.py build --depth 13          # single build, just measure it
"""

import argparse
import statistics
import subprocess
import sys
from pathlib import Path

try:
    import psutil
except ImportError:
    psutil = None


def bench_once(exe: Path, depth: int, cpu: int, timeout: int = 900):
    """Run one bench pinned to `cpu`. Returns (nodes, nps)."""
    proc = subprocess.Popen(
        [str(exe)],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.DEVNULL,
        text=True,
        cwd=str(exe.parent),
    )

    if psutil is not None:
        try:
            p = psutil.Process(proc.pid)
            if cpu >= 0:
                p.cpu_affinity([cpu])
            p.nice(psutil.HIGH_PRIORITY_CLASS if sys.platform == "win32" else -10)
        except Exception as e:  # affinity/priority is best-effort, never fatal
            print(f"    (could not pin/prioritize: {e})")

    out, _ = proc.communicate(f"bench {depth}\nquit\n", timeout=timeout)

    for line in out.splitlines():
        t = line.split()
        if len(t) >= 4 and t[1] == "nodes" and t[3] == "nps":
            return int(t[0]), int(t[2])
    raise RuntimeError(f"could not parse bench output from {exe}:\n{out[-1500:]}")


def find_exe(build_dir: Path) -> Path:
    for name in ("PioneerV4.exe", "PioneerV4"):
        p = build_dir / name
        if p.exists():
            return p.resolve()
    raise SystemExit(f"no engine binary in {build_dir}")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("builds", nargs="+", help="one or two build directories")
    ap.add_argument("--depth", type=int, default=13)
    ap.add_argument("--rounds", type=int, default=6)
    ap.add_argument("--cpu", type=int, default=0,
                    help="logical CPU to pin to (-1 disables pinning). On Intel hybrid parts the "
                         "low indices are usually P-core threads.")
    args = ap.parse_args()

    if psutil is None:
        print("WARNING: psutil not installed -- no pinning, results will be noisy.\n")

    exes = [find_exe(Path(b).resolve()) for b in args.builds]
    labels = [Path(b).name for b in args.builds]
    samples = {l: [] for l in labels}
    nodes = {}

    print(f"depth {args.depth}, {args.rounds} rounds, pinned to CPU {args.cpu}\n")
    for r in range(args.rounds):
        line = f"round {r + 1}: "
        for label, exe in zip(labels, exes):
            n, nps = bench_once(exe, args.depth, args.cpu)
            samples[label].append(nps)
            nodes[label] = n
            line += f"{label}={nps:>9}  "
        print(line, flush=True)

    print()
    for label in labels:
        v = samples[label]
        print(f"{label:>22}: best={max(v):>9}  median={int(statistics.median(v)):>9}  "
              f"spread={100 * (max(v) - min(v)) / max(v):>5.1f}%")

    if len(labels) == 2:
        a, b = labels
        ba, bb = max(samples[a]), max(samples[b])
        wins = sum(1 for x, y in zip(samples[a], samples[b]) if y > x)
        print(f"\n  best-of-{args.rounds} delta ({b} vs {a}): {100 * (bb - ba) / ba:+.1f}%")
        print(f"  {b} faster in {wins}/{args.rounds} rounds")
        if nodes[a] != nodes[b]:
            print(f"  WARNING: node counts differ ({nodes[a]} vs {nodes[b]}) -- "
                  f"this is a behaviour change, not just a throughput change.")
        else:
            print(f"  node counts identical ({nodes[a]}) -- behaviour unchanged.")

        spread = max(100 * (max(samples[l]) - min(samples[l])) / max(samples[l]) for l in labels)
        if spread > 8:
            print(f"\n  CAUTION: run-to-run spread is {spread:.0f}%, which is large relative to "
                  f"typical\n  optimization effects. Close background applications and re-run "
                  f"before trusting\n  a difference smaller than that.")


if __name__ == "__main__":
    sys.exit(main())
