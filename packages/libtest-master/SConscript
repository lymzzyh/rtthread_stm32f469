import os
from building import * 

# get current dir path
cwd = GetCurrentDir()

# init src and inc vars
src = []
inc = []

# add lib common include
inc = inc + [cwd]

# add lib basic code
src = src + ['./libtest.c']

# add group to IDE project
objs = DefineGroup('libtest-1.0.0', src, depend = ['PKG_USING_LIBTEST'], CPPPATH = inc)

# traversal subscript
list = os.listdir(cwd)
if GetDepend('PKG_USING_LIBTEST'):
    for d in list:
        path = os.path.join(cwd, d)
        if os.path.isfile(os.path.join(path, 'SConscript')):
            objs = objs + SConscript(os.path.join(d, 'SConscript'))

Return('objs') 
