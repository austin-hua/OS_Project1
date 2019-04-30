#!/usr/bin/env python

"""
A simple program that calculates the run time of each process in experiment 1

This program reads the input line by line.
Example usage:
    dmesg | grep '\\[Project1\\]' | tail -n30 | ./time_calc.py | sort -g
"""
from itertools import dropwhile, takewhile
from copy import copy

def line_filter(strings):
    important_part = dropwhile(lambda x:x != '[Project1]', strings)
    next(important_part) # drop first item
    return important_part

class TimeInfo:
    def __init__(self, string):
        sec, nano_sec = string.split('.')
        self.sec = int(sec) 
        self.nsec = int(nano_sec)
        leading_zeroes = len(nano_sec) - len(str(self.nsec))
        self.nsec /= pow(10, leading_zeroes)

    def __sub__(self, other):
        if not isinstance(other, TimeInfo):
            return NotImplemented
        res = copy(self)
        res.sec -= other.sec
        res.nsec -= other.nsec 
        if res.nsec < 0:
            res.nsec += pow(10,9)
            res.sec -= 1
        return res

    def __str__(self):
        
        return str(self.sec + self.nsec / 10.0)

if __name__ == '__main__':
    try:
        while True:
            line = input()
            pid, begin, end = line_filter(line.split())
            print(pid, ': ' , TimeInfo(end) - TimeInfo(begin))

            
    except EOFError:
        pass
