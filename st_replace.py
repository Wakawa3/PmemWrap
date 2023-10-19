import re
import sys
import subprocess

if __name__ == "__main__":
    f = open('countfile_plus.txt', 'r')
    # exfile = re.compile(".*_\+\+")
    # pos = re.compile("_\+\+.*")

    out_f = open('countfile_stack.txt', 'w')

    s = f.read()
    lines = s.split('\n')

    i = 0
    flag = False
    while i < len(lines):
        if "_++" in lines[i]:
            addr2line_args = lines[i].split("_++")
            cmd = ["addr2line", "-f", "-e", addr2line_args[0], addr2line_args[1]]
            out = subprocess.run(cmd, capture_output=True, text=True).stdout
            if out == None:
                print("None: " + lines[i] + "\n")
            else:
                out_set = out.split("\n")
                out_f.write(out_set[1] + "    " + out_set[0] + "\n")
        else:
            out_f.write(lines[i] + "\n")
        
        i += 1

    f.close()
    out_f.close()