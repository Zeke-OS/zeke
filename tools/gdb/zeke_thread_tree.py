import gdb

class threadTreePrint(gdb.Command):
    "Print thread tree of a given thread"

    def __init__(self):
        super(threadTreePrint, self).__init__("print-thread-tree", 
                gdb.COMMAND_DATA, gdb.COMPLETE_SYMBOL)

    def invoke(self, arg, from_tty):
        args = gdb.string_to_argv(arg)
        
        try:
            th = gdb.parse_and_eval(args[0])
        except:
            gdb.write('No symbol "%s" in current context.\n' % str(args[0]))
            return

        print nodeToString(th, 0)

def nodeToString(th, d):
        if th.address == 0:
            return 'NULL\n'

        s = '-' + str(th['id']) + '\n'

        inh = th['inh']
        child = inh['first_child'].dereference()
        if child.address != 0:
            s += (d + 1) * ' ' + '|'
            s += nodeToString(child, d + 1)
        next_child = inh['next_child'].dereference()
        if next_child.address != 0:
            s += d * ' ' + '|'
            s += nodeToString(next_child, d)
        else:
            if d >= 3:
                s += (d - 3) * ' ' + '|\n'

        return s

threadTreePrint()
