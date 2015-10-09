"""
The MIT License (MIT)

Copyright (c) 2015 Terence Parr, Hanzhou Shi, Shuai Yuan, Yuanyuan Zhang

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
"""

# simple program to convert addresses like 0x7fc49a404b80 to unique index from 0
# reads in stuff like:
#
#   malloc 56 -> 0x7fc49a500c00
#   free 0x7fc49a500c00
#
# and emits
#
#   malloc 56 -> 0
#   free 0
#

import sys

addr2index = {}
next = 0

for line in sys.stdin:
    words = line.strip().split(" ")
    if len(words)==0: break
    if words[0] == "malloc":
        addr = words[3]
        if addr not in addr2index:
            addr2index[addr] = str(next)
            next += 1
        words[3] = addr2index[addr]
        print " ".join(words)
    else: # must be free
        addr = words[1]
        if addr in addr2index: # ignore frees of unmatched allocs
            words[1] = addr2index[addr]
            print " ".join(words)
        else:
            print "#", " ".join(words) # put a comment symbol in front to indicate not to exec but keep in output
