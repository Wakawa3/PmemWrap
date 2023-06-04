import re
import sys
import subprocess

class OutputData:
    def __init__(self):
        self.number = 0
        self.line = -1
        self.source = ""
        self.timeout = 0
        self.pre_asan_outputs = []
        self.post_asan_outputs = []

if __name__ == "__main__":
    f = open(sys.argv[1], 'r')
    exfile = re.compile(".*_\+\+")
    pos = re.compile("_\+\+.*")

    s = f.read()
    lines = s.split('\n')

    i = 0
    flag = False
    while i < len(lines):
        if len(lines[i]) == 0:
            i += 1
            continue

        elif lines[i][0:6] == "+stack":
            flag = True
            print(lines[i])

        elif lines[i][0] == ';':
            flag = False
            print(lines[i])

        else:
            if flag == False:
                print(lines[i])
            else:
                ef = exfile.search(lines[i])
                p = pos.search(lines[i])
                
                cmd = ["addr2line", "-f", "-e", ef.group()[0:-3], p.group()[3:]]
                out = subprocess.check_output(cmd)
                out_s = str(out)[:-3].replace("\\n", ": ").replace("b\'", "")
                print(out_s)

        i += 1


    f.close()