import cProfile
import re
import itertools
import sys
from math import *
import fileinput
import io
import traceback
import resource
import time
from collections import deque

# We want stack pls
resource.setrlimit(resource.RLIMIT_STACK, (resource.RLIM_INFINITY, resource.RLIM_INFINITY))
sys.setrecursionlimit(50000)

class SynErr(Exception):
    def __init__(self, line,sofar,togo):
        self.line = line
        self.sofar = sofar
        self.togo = togo

ENDC = "ENDC"
REPEAT = "REPEAT"
DOWN = "DOWN"
UP = "UP"
COLOR = "COLOR"
FORW = "FORW"
LEFT = "LEFT"
RIGHT = "RIGHT"
BACK = "BACK"
BEGIN = "BEGIN"

# lex string -> [(line, [token])]
# type tokenl = [(line, [token])]
def lex(s):
    # We really don't wanna deal with the empty case in main code
    if s == "":
        return [(0,[])]
    word = ""
    result = []
    words = deque()
    line = 1
    r = 0
    newLine = False
    iterator = iter(s)
    for c in iterator:
        if c == "%": # Just munch it up until we hit a newline.
            # We currently let c == \n branch handle the empty words case. This isn't super pretty, but whatever
            if word != "":
                words.append(word.upper())
            word = ""
            while c != "\n":
                try:
                    c = iterator.__next__()
                except StopIteration:
                    break
        # The space/tab case shows exactly how this is supposed to be handled for EVERY CASE
        if c == " " or c == "\t": 
            if word != "":
                words.append(word.upper())
            word = ""
        elif c == ".":
            if word != "":
                words.append(word.upper())
            words.append(ENDC)
            word = ""
        elif c == "\"":
            if word != "":
                # Ok we have a word here
                # That means that last character was NOT a space, \n or whatever
                # That means that next character MUST be a space. (because we need space between args)
                # Now we have two choices: introduce a state variable flag
                # Or 'peek' and make a new iterator afterwards filling in the peeked value
                # First is fast and buggy, latter is slow and sure to work
                # Let's go with latter first
                words.append(word.upper())
                peek = iterator.__next__()
                if(peek != " " and peek != "\n" and peek != "\t"):
                    raise SynErr(line, None, None)
                else:
                    iterator = itertools.chain([peek], iterator)
            word = ""
            words.append(REPEAT)
        elif c == "\n":
            if word != "":
                words.append(word.upper())
            word = ""
            if words != deque(): # Handle Whole-line comment
                result.append((line, words))
            words = deque()
            line += 1
        else:
            word += c
    # This is in case that our program didn't end with a special case character
    # This really only happens with a single-literal program such as 123 or #FF
    # I don't even know if those are legal programs
    if word != "": 
        words.append(word.upper())
    if words != deque():
        result.append((line, words))
    return result

# Grammar:
# program := command | command program
# command := 0ary | 1ary | 2ary | 
# 0ary := DOWN ENDC | UP ENDC
# 1ary := COLOR color ENDC | FORW number ENDC | BACK number ENDC | LEFT number ENDC | RIGHT number ENDC
# 2ary := REP integer REPEAT program REPEAT | REP integer command
# integer := [0-9]*
# color := #[0-9A-F]+

# An is function takes one value and produces two return values:
# The value provided is of type [(line, [token])]
# The return values are:
# 0. AST-node for success, None for failure
# 1. The line the token came from
# On success it manipulated the tokenl passed in
# Convention: If we have a tokenl called x, then a new tokenl with something removed is called xx
# Example: ttokenl = tokenl[:0]

def peek_token(tokenl):
    rline = 0
    for (line, tokens) in tokenl:
        rline = line
        for token in tokens:
            return token, line
    return None, rline

def next_token(tokenl):
    rline = 0
    remove_queue = []
    return_val = None
    for line, tokens in tokenl:
        rline = line
        if len(tokens) > 0:
            return_val = tokens.popleft(), line
            break
        else: # Remove the empty line
            remove_queue.append(line)
    for l in remove_queue:
        tokenl.remove((l, deque()))
    if return_val != None:
        return return_val
    else:
        return None, rline

def isprogram(tokenl,end_token=""):
    astacc = []
    myast = [BEGIN]
    whocares, oline = peek_token(tokenl)
    while True: # Don't worry, we'll break
        node, line = iscommand(tokenl)
        if node == None: # OK, we might be done
            node, l1ine = peek_token(tokenl)
            if node == REPEAT and end_token == REPEAT: # Aah, we did work for is2ary()
                myast.extend(astacc)
                node, line = next_token(tokenl) # Munch that END token up
                return myast, oline
            elif end_token == REPEAT and node != REPEAT: # OK, we probably ended up with an empty node again
                raise SynErr(l1ine,tokenl, tokenl)
            elif node == None: # Ok, we are simply done!
                myast.extend(astacc)
                return myast, oline
            else: # Something bad happened, we have tokens we can't deal with
                raise SynErr(l1ine, tokenl, tokenl) # Note: not raising at oline
        else:
            astacc.append(node)

def iscommand(tokenl):
    node = None
    line = None
    node, line = is0ary(tokenl)
    if node != None:
        return node, line
    node, line = is1ary(tokenl)
    if node != None:
        return node, line
    node, line = is2ary(tokenl)
    if node != None:
        return node, line
    return None, line
# 0ary := DOWN ENDC | UP ENDC
def is0ary(tokenl):
    com, oline = peek_token(tokenl)
    if com ==  DOWN or com == UP:
        com, oline = next_token(tokenl)
        endc, line = peek_token(tokenl) 
        if endc == ENDC:
            endc, oline = next_token(tokenl)
            return [com], oline
        else:
            if endc == None: # Crap, we ran out of tokens
                raise SynErr(oline, tokenl, tokenl) # Return our line
            else: # Simply bad token
                raise SynErr(oline, tokenl, tokenl) # Return line for faulty token
    else:
        return None, oline

# 1ary := COLOR color ENDC | FORW number ENDC | BACK number ENDC | LEFT number ENDC | RIGHT number ENDC
def is1ary(tokenl):
    com, oline = peek_token(tokenl)
    if com == COLOR:
        com, oline = next_token(tokenl)
        arg, line = iscolor(tokenl)
        if arg == None:
            raise SynErr(line, tokenl, tokenl)
        else:
            endc, line = next_token(tokenl)
            if endc == None or endc != ENDC:
                raise SynErr(line, tokenl, tokenl)
            else: # Ok, we're finally through!
                return [com, arg], oline
    elif com == FORW or com == BACK or com == LEFT or com == RIGHT:
        com, oline = next_token(tokenl)
        arg, line = isinteger(tokenl)
        if arg == None:
            raise SynErr(line, tokenl, tokenl)
        else:
            endc, line = next_token(tokenl)
            if endc == ENDC:
                return [com,arg], oline
            else:
                raise SynErr(line, tokenl, tokenl)
    else:
        return None, oline        

# 2ary := REP number REPEAT program REPEAT
def is2ary(tokenl):
    com, oline  = peek_token(tokenl)
    if com == "REP":
        com, oline = next_token(tokenl) # OK munch the com
        reps, line = isinteger(tokenl) # Next munch the integer
        if reps == None:
            raise SynErr(line, tokenl, tokenl)
        else:
            repeat, line = peek_token(tokenl) # Case: Several statements
            if repeat == REPEAT:
                repeat, line = next_token(tokenl) # Munch it
                check_2nd, line_ = peek_token(tokenl) # We wanna make sure that we have at least 1 command inside
                if(check_2nd == REPEAT): # We need at least 1 statements!
                    raise SynErr(line_, tokenl, tokenl)
                # Here we use isprogram()
                # Remember: We decided to let isprogram() in on the secret of REPEAT tokens
                # This is bad and sad. Anyway, that's why we pass in REPEAT as a second argument
                programast, line = isprogram(tokenl, REPEAT)
                if programast == None:
                    raise SynErr(line, tokenl, tokenl)
                else:
                    return [REPEAT, reps, programast], oline
            com, line2 = iscommand(tokenl) # Case: Only 1 statement, REPEAT optional
            if com != None:
                not_repeat, line = peek_token(tokenl)
                #if not_repeat == REPEAT: # You can't skip one quote then have a quote after that!!!
                #    raise SynErr(line, tokenl, tokenl)
                #else:
                return [REPEAT, reps, [BEGIN, com]], oline
            else:
                raise SynErr(line, tokenl, tokenl)
    else:
        return None, oline

# These are easy: Consume one token if appropriate, otherwise leave it 
def isinteger(tokenl):
    tok, line = peek_token(tokenl)
    integerr = "^([0-9]*)$"
    if tok == None:
        return None
    if re.search(integerr, tok) != None:
        if tok != "0" : # 0 not allowed, this is actually pretty much a syntax error now
            tok,line = next_token(tokenl)
            return tok, line
        else:
            return None, line
    else:
        return None, line
def iscolor(tokenl):
    tok, line = peek_token(tokenl)
    colorr = "^(#[0-9A-F]*)$"
    if re.search(colorr, tok) != None:
        tok,line = next_token(tokenl)
        return tok, line
    else:
        return None, line

def car(ast):
    if len(ast) > 0:
        return ast[0]
    return []
def cdr(ast):
    return ast[1:]
def cddr(ast):
    return ast[1:][1:]
def caddr(ast):
    return cddr(ast)[0]
def cadr(ast):
    return ast[1:][0]

pendown = False
x = 0
y = 0
angle = 0
color = "#0000FF"
def leval(ast):
    global pendown, x, y, angle, color
    head = car(ast)
    if head == []:
        return
    if head == BEGIN: # leval() in order
        dast = cdr(ast)
        while dast != []:
            leval(car(dast))
            dast = cdr(dast)
        #list(map(leval, cdr(ast))) # list around to force eval
    elif head == REPEAT:
        reps = cadr(ast)
        i = 0
        prog = caddr(ast)
        while(i < int(reps)):
            leval(prog)
            i += 1
    elif head == DOWN:
        pendown = True
        leval(cdr(ast))
    elif head == UP:
        pendown = False
        leval(cdr(ast))
    elif head == COLOR:
        color = cadr(ast)
        leval(cddr(ast))
    elif head == FORW:
        forw(int(cadr(ast)))
        leval(cddr(ast))
    elif head == LEFT:
        left(int(cadr(ast)))
        leval(cddr(ast))
    elif head == RIGHT:
        right(int(cadr(ast)))
        leval(cddr(ast))
    elif head == BACK:
        back(int(cadr(ast)))
        leval(cddr(ast))
def print_x():
    global pendown, x, y, angle, color
    print("{0:0=3d}".format(abs(round(x))), end="")
def print_y():
    global pendown, x, y, angle, color
    print("{0:0=3d}".format(abs(round(y))), end="")
def forw(n):
    global pendown, x, y, angle, color
    if pendown:
        print_x()
        print_y()
    x = x + n * cos((pi * angle)/180)
    y = y + n * sin((pi * angle)/180)
    if pendown:
        print_x()
        print_y()
def left(a):
    global pendown, x, y, angle, color
    angle += a
def right(a):
    global pendown, x, y, angle, color
    angle -= a
def back(n):
    global pendown, x, y, angle, color
    forw(-n)

def run(program):
    global pendown, x, y, angle, color
    pendown = False
    x = 75
    y = 15
    angle = 0
    color = "#0000FF"
    l = lex(program)
    p,whatever = isprogram(l)
    leval(p)

#connection = open("/dev/ttyUSB0", mode='ra', encoding='ascii')
def main():
    prog = io.StringIO()
    for line in sys.stdin:
        prog.write(line)
    try:
        run(prog.getvalue())
    except SynErr as err:
        print("Syntaxfel på rad {}".format(err.line))

def bench():
    prog = io.StringIO()
    prog.write("DOWN.")
    for i in range(20000):
        prog.write("REP 1 \n")
    prog.write("FORW 1.")
    start_time = time.time()
    l = lex(prog.getvalue())
    print(time.time() - start_time)
    length = len(l[0][1])
    start_time = time.time()
    r, l = isprogram(l)
    delta = time.time() - start_time
    print("{} instructions/sec".format(length/delta))
    start_time = time.time()
    leval(r)
    print(time.time() - start_time)
def profile():
    cProfile.run("bench()")

if __name__ == "__main__":
    main()
