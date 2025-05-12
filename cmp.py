mine = open("m.txt", "r").read()
stock = open("s.txt", "r").read()

mine = mine.split("\n")
stock = stock.split("\n")

moves = dict()
for i in range(len(mine)):
    move, val = mine[i].split(": ")
    moves[move] = [val]

for i in range(len(stock)):
    move, val = stock[i].split(": ")
    if move in moves:
        moves[move].append(val)
    else:
        moves[move] = ["", val]


for move in moves:
    if len(moves[move]) == 1:
        print(f"{move}: (M){moves[move][0]} -> (S)None")
    elif moves[move][0] != moves[move][1]:
        print(f"{move}: (M){moves[move][0]} -> (S){moves[move][1]}")
