import re
import sys
import subprocess

class OutputData:
    def __init__(self):
        self.number = 0
        self.line = -1
        self.source = ""
        self.timeout = 0
        self.pmemwrap_stacktrace = ""
        self.pre_asan_outputs_lines = []
        self.post_asan_outputs_lines = []

#citestから次のcitestまでがdata
#citest内で=========で始まる部分をそれぞれoutputとする

if __name__ == "__main__":
    f = open(sys.argv[1], 'r')
    asan_stacktrace_start_pattern = re.compile("\s*#0[\d\D]*")
    start_asan_str = "================================================================="
    asan_pattern1 = re.compile("==[0-9]+==")
    start_state = 0
    pre_failure_state = 1
    post_failure_state = 2

    s = f.read()
    lines = s.split('\n')

    data_list = []

    state = start_state
    i = 0
    new_data = None
    while i < len(lines):
        if state == start_state:
            if lines[i].startswith("citest_"):
                new_data = OutputData()
                new_data.number = int(lines[i][7:])
                state = pre_failure_state

        elif state == pre_failure_state:
            if lines[i] == start_asan_str:
                new_data.pre_asan_outputs_lines.append([])
            elif isinstance(asan_pattern1.match(lines[i]), re.Match):
                if not i in new_data.pre_asan_outputs_lines:
                    new_data.pre_asan_outputs_lines[-1].append(i)
            elif isinstance(asan_stacktrace_start_pattern.match(lines[i]), re.Match):
                l = i
                while l < len(lines):
                    if lines[l] == '':
                        for x in range(i-2, l):
                            if not x in new_data.pre_asan_outputs_lines:
                                new_data.pre_asan_outputs_lines[-1].append(x)
                        i = l + 1
                        break
                    l += 1
                else:
                    print("Error: sema_state_generate.py", file=sys.stderr)
                    sys.exit(1)
            elif "_++" in lines[i]:
                while "_++" in lines[i]:
                    addr2line_args = lines[i].split("_++")
                    cmd = ["addr2line", "-fe", addr2line_args[0], addr2line_args[1]]
                    out = subprocess.run(cmd, capture_output=True, text=True).stdout
                    if out == None:
                        print("None: " + lines[i] + "\n")
                    else:
                        out_set = out.split("\n")
                        new_data.pmemwrap_stacktrace += out_set[1] + "    " + out_set[0] + "\n"
                    i += 1
            elif lines[i].startswith("set abortflag file: "):
                tmp = lines[i].find(',', 20) #１つ目の','の位置
                new_data.source = lines[i][20:tmp]
                tmp2 = lines[i].find('line: ', tmp) + 6
                tmp3 = lines[i].find(',', tmp2)
                new_data.line = int(lines[i][tmp2:tmp3])
            elif lines[i] == "post_failure":
                state = post_failure_state

        elif state == post_failure_state:
            if lines[i] == start_asan_str:
                new_data.post_asan_outputs_lines.append([])
            elif isinstance(asan_pattern1.match(lines[i]), re.Match):
                if i in new_data.post_asan_outputs_lines:
                    new_data.post_asan_outputs_lines[-1].append(i)
            elif isinstance(asan_stacktrace_start_pattern.match(lines[i]), re.Match):
                l = i
                while l < len(lines):
                    if lines[l] == '':
                        for x in range(i-2, l):
                            if not x in new_data.post_asan_outputs_lines:
                                new_data.post_asan_outputs_lines[-1].append(x)
                        i = l + 1
                        break
                    l += 1
                else:
                    print("Error: sema_state_generate.py", file=sys.stderr)
                    sys.exit(1)

            elif lines[i].startswith("timeout return "):
                new_data.timeout = int(lines[i][15:])
                data_list.append(new_data)
                state = start_state
        i += 1

    num16 = re.compile("0x[a-f0-9]{12}")  

    pre_combined_asan_outputs = [] #各出力　pre_crash_pos、pmemwrap_stacktraceと番号共有
    pre_crash_pos = [] # 行、ソースファイル

    for data in data_list:
        for output in data.pre_asan_outputs_lines:
            output.sort()
            new_output = []
            new_output_str = ""
            for line in output:
                new_output.append(lines[line])
                new_output[-1] = num16.sub("0x____________", new_output[-1])
                new_output[-1] = asan_pattern1.sub("==______==", new_output[-1])
            new_output_str = "\n".join(new_output)
            for j, combined_asan_output in enumerate(pre_combined_asan_outputs):# combined_asan_outputは各出力
                if new_output_str == combined_asan_output:
                    for k in range(len(pre_crash_pos[j])):
                        if pre_crash_pos[j][k]["line"] == data.line and pre_crash_pos[j][k]["source"] == data.source:
                            break
                    else:
                        pre_crash_pos[j].append({"line": data.line, "source": data.source})
                    break
            else:
                pre_combined_asan_outputs.append(new_output_str)
                pre_crash_pos.append([])
                pre_crash_pos[-1].append({"line": data.line, "source": data.source})

    post_combined_asan_outputs = []
    post_crash_pos = []
    post_pmemwrap_stacktrace = []

    for data in data_list:
        for output in data.post_asan_outputs_lines:
            output.sort()
            new_output = []
            new_output_str = ""
            for line in output:
                if lines[line] != start_asan_str:
                    new_output.append(lines[line])
                    new_output[-1] = num16.sub("0x____________", new_output[-1])
                    new_output[-1] = asan_pattern1.sub("==______==", new_output[-1])
            new_output_str = "\n".join(new_output)
            for j, combined_asan_output in enumerate(post_combined_asan_outputs):# combined_asan_outputは各出力
                if new_output_str == combined_asan_output:
                    for k in range(len(post_crash_pos[j])):#出力に対応するpost_crash_pos要素
                        if post_crash_pos[j][k]["line"] == data.line and post_crash_pos[j][k]["source"] == data.source:
                            break
                    else:
                        post_crash_pos[j].append({"line": data.line, "source": data.source})
                    
                    for k in range(len(post_pmemwrap_stacktrace[j])):#lenはstacktraceの数
                        if data.pmemwrap_stacktrace == post_pmemwrap_stacktrace[j][k]:
                            break
                    else:
                        post_pmemwrap_stacktrace[j].append(data.pmemwrap_stacktrace)

                    break

            else: #同じ出力がなかった場合
                post_combined_asan_outputs.append(new_output_str)
                post_crash_pos.append([])
                post_crash_pos[-1].append({"line": data.line, "source": data.source})
                post_pmemwrap_stacktrace.append([])
                post_pmemwrap_stacktrace[-1].append(data.pmemwrap_stacktrace)
                

    print("pre_failure");
    for i, combined_asan_output in enumerate(pre_combined_asan_outputs):
        # print("\n\nline:", end=" ")
        for j in range(len(pre_crash_pos[i])):
            print(pre_crash_pos[i][j], end=", ")
        print("")
        print(start_asan_str)
        print(combined_asan_output)
        print("")

    print("|||||||||||||||||||||||||||||||||||||||||||")

    print("post_failure");
    for i, combined_asan_output in enumerate(post_combined_asan_outputs):
        # print("\n\nline:", end=" ")
        for j in range(len(post_crash_pos[i])):
            print(post_crash_pos[i][j])
        for j in range(len(post_pmemwrap_stacktrace[i])):
            print("stack_" + str(j) + "\n" + post_pmemwrap_stacktrace[i][j])
            
        print("")
        print(start_asan_str)
        print(combined_asan_output)
        print("")
    f.close()