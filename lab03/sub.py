#Tate Murphy
#264542
import re

def main():
    letters = open("letters.txt", "r")
    mids = open("mids.txt", "w")
    for line in letters:
        l = []
        subs = []
<<<<<<< HEAD
        regex = r"^(?=.*m.*i.*d.*s)"
=======
        regex = r"m+i+d+s+"
>>>>>>> d3991a2127cf2398032fa436b686c0ca42d52195
        contains = False
        for letter in line:
            if letter == 'M':
                l.append('m')
                subs.append('m')
            elif letter == 'I':
                l.append('i')
                subs.append('i')
            elif letter == 'D':
                l.append('d')
                subs.append('d')
            elif letter == 'S':
                l.append('s')
                subs.append('s')
            else:
                l.append(letter)
            word = "".join(subs)
<<<<<<< HEAD
        if re.search(regex, word):
=======
        if re.fullmatch(regex, word):
>>>>>>> d3991a2127cf2398032fa436b686c0ca42d52195
             contains = True
        if contains == True:
            mids.write("".join(l))
            


main()
