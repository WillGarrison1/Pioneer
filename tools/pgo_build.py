#!/usr/bin/env python3
"""
Profile-guided optimization build for Pioneer.

Runs the full three-stage PGO cycle:

    1. build instrumented   (-fprofile-generate)
    2. run `bench`          -> writes profile counters on clean exit
    3. rebuild optimized    (-fprofile-use)

Both compile stages deliberately share one build directory: GCC mangles each object file's
absolute path into the profile filename, so building the two stages in different directories
makes the USE stage silently find no profile and produce a plain -O3 binary.

Usage:
    python tools/pgo_build.py [--build-dir build-pgo] [--depth 13] [--compare]

--compare additionally builds a plain Release binary and benches both, so you can see what PGO
actually bought on your machine rather than assuming.
"""

import argparse
import shutil
import subprocess
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent


def run(cmd, **kw):
    print(f"  $ {' '.join(str(c) for c in cmd)}")
    r = subprocess.run(cmd, **kw)
    if r.returncode != 0:
        print(f"command failed with exit {r.returncode}")
        sys.exit(r.returncode)
    return r


def configure(build_dir, pgo_stage, generator, extra=None):
    cmd = [
        "cmake", "-S", str(ROOT), "-B", str(build_dir),
        "-G", generator,
        "-DCMAKE_BUILD_TYPE=Release",
        f"-DPIONEER_PGO={pgo_stage}",
    ]
    if extra:
        cmd += extra
    run(cmd, stdout=subprocess.DEVNULL)


def build(build_dir, jobs):
    run(["cmake", "--build", str(build_dir), "-j", str(jobs)], stdout=subprocess.DEVNULL)


def engine_path(build_dir):
    for name in ("PioneerV4.exe", "PioneerV4"):
        p = build_dir / name
        if p.exists():
            return p
    print(f"engine binary not found in {build_dir}")
    sys.exit(1)


def run_bench(exe, depth, label):
    """Run bench and return (nodes, nps). Requires a clean exit so profile data is flushed."""
    print(f"  running bench depth {depth} ({label}) ...")
    start = time.time()
    proc = subprocess.run(
        [str(exe)],
        input=f"bench {depth}\nquit\n",
        capture_output=True,
        text=True,
        timeout=1800,
        cwd=str(exe.parent),
    )
    elapsed = time.time() - start

    nodes = nps = None
    for line in proc.stdout.splitlines():
        parts = line.split()
        if len(parts) >= 4 and parts[1] == "nodes" and parts[3] == "nps":
            nodes, nps = int(parts[0]), int(parts[2])

    if nodes is None:
        print("  could not parse bench output:")
        print(proc.stdout[-2000:])
        sys.exit(1)

    print(f"  -> {nodes} nodes, {nps} nps  ({elapsed:.1f}s wall)")
    return nodes, nps


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--build-dir", default="build-pgo")
    ap.add_argument("--depth", type=int, default=13,
                    help="bench depth used as the training workload")
    ap.add_argument("--jobs", type=int, default=8)
    ap.add_argument("--generator", default="MinGW Makefiles")
    ap.add_argument("--compare", action="store_true",
                    help="also build plain Release and report the speedup")
    ap.add_argument("--clean", action="store_true",
                    help="delete the build dir first (recommended if flags changed)")
    args = ap.parse_args()

    build_dir = (ROOT / args.build_dir).resolve()
    if args.clean and build_dir.exists():
        print(f"removing {build_dir}")
        shutil.rmtree(build_dir)

    # Stale counters from an earlier run would be merged into this one, so clear them.
    # GCC writes .gcda next to each object file; Clang writes .profraw into pgo-data.
    pgo_data = build_dir / "pgo-data"
    if pgo_data.exists():
        shutil.rmtree(pgo_data)
    for f in build_dir.rglob("*.gcda"):
        f.unlink()

    print("\n[1/3] building instrumented binary (-fprofile-generate)")
    configure(build_dir, "GENERATE", args.generator)
    build(build_dir, args.jobs)

    print("\n[2/3] training")
    exe = engine_path(build_dir)
    run_bench(exe, args.depth, "instrumented -- expected to be slow")

    profiles = list(build_dir.rglob("*.gcda")) + list(pgo_data.rglob("*.profraw"))
    print(f"  collected {len(profiles)} profile files in {build_dir}")
    if not profiles:
        print("  no profile data was written -- aborting, the USE stage would be a no-op.")
        print("  (the engine must exit cleanly for counters to be flushed)")
        sys.exit(1)

    # Clang needs an explicit merge step; GCC's .gcda files are consumed directly.
    if profiles[0].suffix == ".profraw":
        merged = pgo_data / "pioneer.profdata"
        llvm_profdata = shutil.which("llvm-profdata")
        if not llvm_profdata:
            print("  llvm-profdata not found but clang .profraw files were produced.")
            sys.exit(1)
        run([llvm_profdata, "merge", f"-output={merged}"] + [str(p) for p in profiles])

    print("\n[3/3] rebuilding optimized (-fprofile-use)")
    configure(build_dir, "USE", args.generator)
    # Capture build output: a "profile count data file not found" warning means the USE stage
    # is running blind, which makes the binary slower than plain Release.
    r = subprocess.run(["cmake", "--build", str(build_dir), "-j", str(args.jobs)],
                       capture_output=True, text=True)
    if r.returncode != 0:
        print(r.stdout[-3000:] + r.stderr[-3000:])
        sys.exit(r.returncode)
    missing = (r.stdout + r.stderr).count("profile count data file not found")
    if missing:
        print(f"  WARNING: {missing} missing-profile warnings -- PGO data was not applied.")

    pgo_nodes, pgo_nps = run_bench(engine_path(build_dir), args.depth, "PGO")

    if args.compare:
        plain_dir = (ROOT / (args.build_dir + "-plain")).resolve()
        print("\n[compare] building plain Release")
        configure(plain_dir, "OFF", args.generator)
        build(plain_dir, args.jobs)
        base_nodes, base_nps = run_bench(engine_path(plain_dir), args.depth, "plain Release")

        print("\n" + "=" * 58)
        print(f"  plain Release : {base_nps:>10} nps")
        print(f"  PGO           : {pgo_nps:>10} nps")
        print(f"  speedup       : {100.0 * (pgo_nps - base_nps) / base_nps:>9.1f} %")
        if base_nodes != pgo_nodes:
            print(f"  WARNING: node counts differ ({base_nodes} vs {pgo_nodes}).")
            print("  PGO must not change search behaviour -- this indicates a real problem.")
        else:
            print(f"  node counts identical ({pgo_nodes}) -- behaviour unchanged, as required.")
        print("=" * 58)

    print(f"\nPGO binary: {engine_path(build_dir)}")


if __name__ == "__main__":
    sys.exit(main())
