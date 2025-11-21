import sys


def read(move: int):
    fr = move & 0x3F
    to = (move >> 6) & 0x3F

    frf = fr % 8
    frr = fr // 8
    tof = to % 8
    tor = to // 8

    a = ord("a")
    one = ord("1")

    moveStr = chr(frf + a) + chr(frr + one) + chr(tof + a) + chr(tor + one)

    piece = (move >> 12) & 0xF

    print(f"Move: {moveStr}\nFrom: {fr}\nTo: {to}\nPiece: {piece}")


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python3 moveRead.py <move>")
        sys.exit(1)
    move = int(sys.argv[1])
    read(move)
