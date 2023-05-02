import re
import sys

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
    stack_trace = re.compile("\s*#0[\d\D]*")
    start_asan_str = "================================================================="
    pre_failure_str = "pre_failure"
    post_failure_str = "post_failure"
    set_abortflag_str = "set abortflag file: "
    timeout_str = "timeout return "

    s = f.read()
    lines = s.split('\n')
    # print(lines)

    data = []

    state = "start"
    i = 0
    while i < len(lines):
        if state == "start":
            if lines[i][0:7] == "citest_":
                new_data = OutputData()
                new_data.number = int(lines[i][7:])
                data.append(new_data)
                state = pre_failure_str
                i += 2 #pre_failureの行の分
                continue
            i += 1

        elif state == pre_failure_str:
            if lines[i] == start_asan_str:
                data[-1].pre_asan_outputs.append([])
            elif lines[i] != post_failure_str and lines[i][:20] != set_abortflag_str:
                tmp = stack_trace.match(lines[i])
                if isinstance(tmp, re.Match):
                    l = i
                    while l < len(lines):
                        if lines[l] == '':
                            new_stack_trace = []
                            for x in range(i-2, l):
                                # print(lines[x])
                                new_stack_trace.append(lines[x])
                            i = l
                            data[-1].pre_asan_outputs[-1].append(new_stack_trace)
                            break
                        l += 1
            elif lines[i][:20] == set_abortflag_str:
                tmp = lines[i].find(',', 20) #１つ目の','の位置
                data[-1].source = lines[i][20:tmp]
                tmp2 = lines[i].find('line: ', tmp) + 6
                tmp3 = lines[i].find(',', tmp2)
                data[-1].line = int(lines[i][tmp2:tmp3])
            else:
                state = post_failure_str
            i += 1

        elif state == post_failure_str:
            if lines[i] == start_asan_str:
                data[-1].post_asan_outputs.append([])
            if lines[i][:15] != timeout_str:
                tmp = stack_trace.match(lines[i])
                if isinstance(tmp, re.Match):
                    l = i
                    while l < len(lines):
                        if lines[l] == '':
                            new_stack_trace = []

                            for x in range(i-1, l):
                                # print(lines[x])
                                new_stack_trace.append(lines[x])
                            i = l
                            # print(new_stack_trace)
                            # print(len(data))
                            data[-1].post_asan_outputs[-1].append(new_stack_trace)
                            break
                        l += 1
            else:
                data[-1].timeout = int(lines[i][15:])
                state = "start"
            i += 1

    # for i in range(len(data)):
    #     print(data[i].number)
    #     print(data[i].line)
    #     print(data[i].source)
    #     for trace in data[i].pre_asan_outputs:
    #         for str in trace:
    #             print(str)
    #     print("post")
    #     for trace in data[i].post_asan_outputs:
    #         for str in trace:
    #             print(str)
    
    num16 = re.compile("0x[a-f0-9]{12}")
    r_equal = re.compile("==[0-9]{4,}==")

    pre_combined_asan_outputs = []
    pre_crash_pos = []
    for test in data:
        for output in test.pre_asan_outputs:
            for i in range(len(output)):
                for l in range(len(output[i])):
                    output[i][l] = num16.sub("0x____________", output[i][l])
                    output[i][l] = r_equal.sub("==______==", output[i][l])
            notequal_flag = 1
            for j, combined_asan_output in enumerate(pre_combined_asan_outputs):
                for i in range(len(combined_asan_output)):
                    for l in range(len(combined_asan_output[i])):
                        if len(output) != len(combined_asan_output) or len(output[i]) != len(combined_asan_output[i]):
                            break
                        elif output[i][l] != combined_asan_output[i][l]:
                            break
                    else:
                        notequal_flag = 0
                        break
                if notequal_flag == 0:
                    for k in range(len(pre_crash_pos[j])):
                        if pre_crash_pos[j][k]["line"] == test.line and pre_crash_pos[j][k]["source"] == test.source:
                            break
                    else:
                        pre_crash_pos[j].append({"line": test.line, "source": test.source})
                    break
            if notequal_flag == 1:
                # print(len(output))
                pre_combined_asan_outputs.append(output)
                pre_crash_pos.append([])
                pre_crash_pos[-1].append({"line": test.line, "source": test.source})

    post_combined_asan_outputs = []
    post_crash_pos = []
    for test in data:
        for output in test.post_asan_outputs:
            for i in range(len(output)):
                for l in range(len(output[i])):
                    output[i][l] = num16.sub("0x____________", output[i][l])
                    output[i][l] = r_equal.sub("==______==", output[i][l])
            notequal_flag = 1
            for j, combined_asan_output in enumerate(post_combined_asan_outputs):
                for i in range(len(combined_asan_output)):
                    for l in range(len(combined_asan_output[i])):
                        if len(output) != len(combined_asan_output) or len(output[i]) != len(combined_asan_output[i]):
                            break
                        elif output[i][l] != combined_asan_output[i][l]:
                            break
                    else:
                        notequal_flag = 0
                        break
                if notequal_flag == 0:
                    for k in range(len(post_crash_pos[j])):
                        if post_crash_pos[j][k]["line"] == test.line and post_crash_pos[j][k]["source"] == test.source:
                            break
                    else:
                        post_crash_pos[j].append({"line": test.line, "source": test.source})
                    break
            if notequal_flag == 1:
                # print(len(output))
                post_combined_asan_outputs.append(output)
                post_crash_pos.append([])
                post_crash_pos[-1].append({"line": test.line, "source": test.source})

    print("pre_failure");
    for i, combined_asan_output in enumerate(pre_combined_asan_outputs):
        # print("\n\nline:", end=" ")
        for j in range(len(pre_crash_pos[i])):
            print(pre_crash_pos[i][j], end=", ")
        print("")
        print(start_asan_str)
        for trace in combined_asan_output:
            for line in trace:
                print(line)
            print("")

    print("|||||||||||||||||||||||||||||||||||||||||||")

    print("post_failure");
    for i, combined_asan_output in enumerate(post_combined_asan_outputs):
        # print("\n\nline:", end=" ")
        for j in range(len(post_crash_pos[i])):
            print(post_crash_pos[i][j], end=", ")
        print("")
        print(start_asan_str)
        for trace in combined_asan_output:
            for line in trace:
                print(line)
            print("")