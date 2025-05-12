# This file shows a bitboard in a human-readable format.

import sys


def printBB(bb):
    for i in range(8):
        for j in range(8):
            if bb & (1 << (i * 8 + j)):
                print("1", end="")
            else:
                print("0", end="")
        print()
    print()


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python3 printBB.py <bitboard>")
        sys.exit(1)
    bb = int(sys.argv[1])
    printBB(bb)
