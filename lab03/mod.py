#Tate Murphy
#264542

def main():
    numbers = open("numbers.txt", "r")
    out0 = open("mod0.txt", "w")
    out1 = open("mod1.txt", "w")
    out2 = open("mod2.txt", "w")
    for line in numbers:
        res = int(line) % 3
        if res == 0:
            out0.write(line)
        elif res == 1:
            out1.write(line)
        else:
            out2.write(line)

main()
