import pexpect, time

import pexpect.popen_spawn

pioneer = pexpect.popen_spawn.PopenSpawn("build/pioneerV4.exe")

pioneer.expect("PioneerV4.0>")
pioneer.sendline("go perft 6")

while True:
    s = pioneer.readline()
    if s == "" or s == 0:
        break
    
    if "Total Moves: " in s.decode():
        break
    print(s)
    
pioneer.sendline("exit")
print(pioneer.exitstatus)