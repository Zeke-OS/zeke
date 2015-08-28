import gdb

class TAILQPrint(gdb.Command):
    "Print a TAILQ\nArgs: <head> <field>"

    def __init__(self):
        super(TAILQPrint, self).__init__("print-tailq", gdb.COMMAND_USER)

    def invoke(self, arg, from_tty):
        args = gdb.string_to_argv(arg)

        if len(args) < 2:
            gdb.write('nr args < 2\n')
            return
        
        try:
            head = gdb.parse_and_eval(args[0])
        except:
            gdb.write('No symbol "%s" in current context.\n' % str(args[0]))
            return

        print getTAILQ(head, args[1])

def getTAILQ(head, field):
    node = head['tqh_first']

    q = []
    while node != 0:
        q.append(str(node.dereference()))
        node = node[field]['tqe_next']

    return q


TAILQPrint()
