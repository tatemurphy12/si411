#!/usr/bin/env python3
# vmSim.py
import sys

# Constants for the various algorithsm we may use
FIFO  = 10
LRU   = 11
AGING = 12

NUM_AGE_BITS = 10   # For AGING, how many bits should each counter hold?


# A 'MemOp' represents a single load or store memory operation (one line of the trace file)
class MemOp:

    def __init__ (self, isStore, addressInHex, instsBetween):
        self.isStore      = int(isStore)
        self.virtAddress  = int(addressInHex, 16)  # assumes is base-16 (hex)
        self.instsBetween = int(instsBetween)
	
    def __str__(self):
        return "%d %08x %d" % (self.isStore, self.virtAddress, self.instsBetween)


# A 'Frame' represents a single physical frame of memory
class Frame:
    def __init__ (self, isValid, vpn):
        self.isValid     = isValid
        self.vpn         = vpn    # vpn set to -1 when frame is not valid
        self.tolu        = 0
        self.counter     = 0
        self.referenced  = 0
        

# Process command line arguments and return (filename, numFrames, alg, debug)
#  [where 'alg' is one of the constants listed above]
# You should NOT need to change this function
def processArguments():
    # Get arguments
    if len(sys.argv) != 5:
        print ("usage: python3 vmSim.py <filename> <numFrames> <algName=FIFO|LRU|AGING> <debug=0|1|2>")
        sys.exit(-1)
    filename  = sys.argv[1]
    numFrames = int(sys.argv[2])
    algName   = sys.argv[3]
    debug     = int(sys.argv[4])

    if not (numFrames > 1):
        print ("Invalid numFrames", numFrames)
        sys.exit(-1)
        
    if algName == "FIFO":
        alg = FIFO
    elif algName == "LRU":
        alg = LRU
    elif algName == "AGING":
        alg = AGING
    else:
        print ("Invalid algName", algName)
        sys.exit(-1)
        
    if not (debug == 0 or debug == 1 or debug == 2 or debug == 3):
        print ("Invalid debug argument ",debug)
        sys.exit(-1)

    print ("Running vmSim on file",filename,"numFrames",numFrames,"alg",algName,"debug",debug)
        
    return (filename, numFrames, alg, debug)



if __name__=="__main__":

    # Get arguments and open trace file
    (filename, numFrames, alg, debug) = processArguments()        
    file = open(filename,'r')
    if not file:
        print ("Error", filename, "not found!")
        sys.exit(-1)
        
    # Create the table to represent physical frames, initially all empty
    frames = []
    for ii in range(numFrames):
        frames.append(Frame(isValid=False, vpn=-1))

    numOps = numFaults = 0

    # Read each line and send to simulator
    for line in file:
        # Update stats and check for early stopping -- you should NOT need to change this part
        numOps = numOps + 1
        if debug >= 2 and numOps >= 10000:      # stop early for debug mode 2
            break

        # Read next memory operation from file -- you should NOT need to change this part
        line_array = line.split()
        memOp = MemOp(line_array[1], line_array[2], line_array[3])   # start at 1 to skip over '#' at start of line

        # Examine virtual address to split into vpn and offset (yes, changes needed here!)
        vpn    = 0    # TODO (for part 1) -- you must compute this "virtual page number" from memOp.virtAddress !
        offset = 0    # TODO (for part 1) -- you must compute this "page offset"         from memOp.virtAddress !
        if debug:
            print ("time %d virtual address %08x  vpn: %06x offset: %03x" % (numOps, memOp.virtAddress, vpn, offset)) 
            
        # TODO (for part 2) -- make changes here to see if page 'vpn' is already in physical memory (look in 'frames')
        # TODO (for part 2) -- handle page fault if not
        # TODO (for part 2) -- update any reference/timing stats for page that was just used        
        
        if debug:
            # Print the physical address
            physAddress = 0   # TODO (for part 2) -- compute physical address here, based what physical frame applies for this memory access
            # TODO (for part 2) -- uncomment this print statement
            # print ("    physical address %04x (pfn: %02x offset: %03x)" % (physAddress, pfn, offset) )
        

        # Print some stats -- you should NOT need to change this part
        if numOps % 100000 == 0:
            print ("faults", numFaults, " of ", numOps, " faultRate: ", float(numFaults) / numOps)

        
    # Print some stats -- you should NOT need to change this part
    print ("faults", numFaults, " of ", numOps, " faultRate: ", float(numFaults) / numOps)

            
    
