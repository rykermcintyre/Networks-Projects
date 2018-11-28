import sys

if len(sys.argv) != 2 and len(sys.argv) != 3:
	print("python tabs.py <input> [output]")
	exit(0)

out = open(sys.argv[1], "r")
out.close()

if len(sys.argv) == 2:
	in_name = sys.argv[1]
	out = open(in_name, "w")
elif len(sys.argv) == 3:
	in_name = sys.argv[1]
	out_name = sys.argv[2]
	out = open(out_name, "w")
else:
	print("python tabs.py <input> [output]")
	exit(0)

f = open(in_name, "r")
linecount = 0
tablen = 4

for line in f:
	linecount += 1
	count = 0
	line = line.rstrip()
	if len(line) > 0 and line[0] == ' ':
		for c in line:
			if c == ' ':
				count += 1
			else:
				break
		if count % tablen == 0:
			for i in range(int(int(count)/int(tablen))):
				out.write("\t")
			write = False
			for c in line:
				if c != ' ':
					write = True
				if write:
					out.write(c)
		elif len(line) > 1 and line[1] == '*':
			out.write(line)
		else:
			print("Line {} bad format".format(linecount))
			out.write(line)
	else:
		out.write(line)
	out.write("\n")

f.close()

out.close()
