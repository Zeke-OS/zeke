import gdb

class QueuePrint(gdb.Command):
    "Print list defined in queue.h"

    def __init__(self):
        super(QueuePrint, self).__init__("print-queue",
                gdb.COMMAND_DATA, gdb.COMPLETE_NONE, True)

class SLISTPrint(gdb.Command):
    "Print SLIST\nArgs: <head> <field>"

    def __init__(self):
        super(SLISTPrint, self).__init__("print-queue slist",
                gdb.COMMAND_DATA, gdb.COMPLETE_EXPRESSION)

    def invoke(self, arg, from_tty):
        args = getArgs(arg)
        if args is None:
            return

        print(getSLIST(args[0], args[1]))

class LISTPrint(gdb.Command):
    "Print LIST\nArgs: <head> <field>"

    def __init__(self):
        super(LISTPrint, self).__init__("print-queue list",
                gdb.COMMAND_DATA, gdb.COMPLETE_EXPRESSION)

    def invoke(self, arg, from_tty):
        args = getArgs(arg)
        if args is None:
            return

        print(getLIST(args[0], args[1]))

class STAILQPrint(gdb.Command):
    "Print STAILQ\nArgs: <head> <field>"

    def __init__(self):
        super(STAILQPrint, self).__init__("print-queue stailq",
                gdb.COMMAND_DATA, gdb.COMPLETE_EXPRESSION)

    def invoke(self, arg, from_tty):
        args = getArgs(arg)
        if args is None:
            return

        print(getSTAILQ(args[0], args[1]))

class TAILQPrint(gdb.Command):
    "Print TAILQ\nArgs: <head> <field>"

    def __init__(self):
        super(TAILQPrint, self).__init__("print-queue tailq",
                gdb.COMMAND_DATA, gdb.COMPLETE_EXPRESSION)

    def invoke(self, arg, from_tty):
        args = getArgs(arg)
        if args is None:
            return

        print(getTAILQ(args[0], args[1]))

def getArgs(arg):
    args = gdb.string_to_argv(arg)

    if len(args) < 2:
        gdb.write('nr args < 2\n')
        return None

    try:
        head = gdb.parse_and_eval(args[0])
    except:
        gdb.write('No symbol "%s" in current context.\n' % str(args[0]))
        return None

    return [head, args[1]]

def getSLIST(head, field):
    return _getQUEUE(head, field, 'slh_first', 'sle_next')

def getLIST(head, field):
    return _getQUEUE(head, field, 'lh_first', 'le_next')

def getSTAILQ(head, field):
    return _getQUEUE(head, field, 'stqh_first', 'stqe_next')

def getTAILQ(head, field):
    return _getQUEUE(head, field, 'tqh_first', 'tqe_next')

def _getQUEUE(head, field, head_field, next_field):
    node = head[head_field]

    q = []
    while node != 0:
        q.append(str(node.dereference()))
        ep_str = '((' + str(node.type) + ')' + str(node.dereference().address) + ')->' + field
        ep = gdb.parse_and_eval(ep_str)
        node = ep[next_field]

    return q


# Register commands
QueuePrint()
SLISTPrint()
LISTPrint()
STAILQPrint()
TAILQPrint()
