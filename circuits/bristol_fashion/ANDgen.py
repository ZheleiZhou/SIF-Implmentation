file = open('ands.txt', 'w', encoding = 'utf-8')
num_ands = 10000000
file.write(str(num_ands))
file.write(" ")
file.write(str(num_ands+2))
file.write("\n")
file.write("2 1 1\n")
file.write("1 1\n")
file.write("\n")

file.write("2 1 0 1 2 AND\n")
for i in range(1,num_ands):
    file.write("2 1 ")
    file.write("0")
    file.write(" ")
    file.write(str(i+1))
    file.write(" ")
    file.write(str(i+2))
    file.write(" ")
    file.write("AND")
    file.write("\n")

file.close()