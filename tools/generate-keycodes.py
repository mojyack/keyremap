# invoke with `curl https://raw.githubusercontent.com/torvalds/linux/master/include/uapi/linux/input-event-codes.h | python3 generate-keycode.py`

def parse_line(line):
    if not line.startswith("#define KEY_") and not line.startswith("#define BTN_"):
        return
    elms = line[8:].split()

    if len(elms) < 2:
        return

    try:
        num = int(elms[1], 0)
    except ValueError:
        return

    return (num, elms[0].lower())

codes = {}
while 1:
    try:
        line =parse_line(input())
        if line == None:
            continue
        codes[line[0]] = line[1]
    except EOFError:
        break

s2c = open("str2code.txt", "w")
s2c.write("// generated with generate-keycodes.py\n")
c2s = open("code2str.txt", "w")
c2s.write("// generated with generate-keycodes.py\n")

for k, v in codes.items():
    s2c.write("if(str == \"{}\") return {};\n".format(v, k))
    c2s.write("case {}: return \"{}\";\n".format(k, v))
