import re

# line = "beeebeeeebeeeeeg"

# s = re.search("[be]*?g$", line)
# print(s.group())


# line = "abcdef"
# line = line[0:5]
# print(line)
# print(len(line))


class OutputData:
    number = 0
    line = -1
    source = ""
    timeout = 0
    pre_stack_traces = []
    post_stack_traces = []

data1 = OutputData()
data2 = OutputData()

data1.pre_stack_traces.append("a")
print(data2.pre_stack_traces)