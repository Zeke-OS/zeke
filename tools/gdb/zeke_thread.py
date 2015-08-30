import gdb
import gdb.printing
import string

# Pretty printers

class thread_infoPrinter:
    "Print thread_info"

    def __init__(self, val):
        self.val = val

    def to_string(self):
        if self.val.address == 0:
            return "NULL"

        th_id = self.val['id']
        th_flags = self.val['flags']
        th_pidown = self.val['pid_owner']
        sched = self.val['sched']
        th_sched_state = sched['state']
        th_sched_pol_flags = sched['policy_flags']
        th_param_policy = self.val['param']['sched_policy']
        th_param_priority = self.val['param']['sched_priority']
        th_retval = self.val['retval']
        th_exit_signal = self.val['exit_signal']

        th = '{id: '            + str(th_id) + \
            ', state: '         + str(th_sched_state) + \
            ', flags: '         + str(th_flags) + \
            ', policy: '        + str(th_param_policy) + \
            ', policy_flags: '  + str(th_sched_pol_flags) + \
            ', priority: '      + str(th_param_priority) + \
            ', pid_owner: '     + str(th_pidown) + \
            '}'

        return th

def build_pretty_printer():
    pp = gdb.printing.RegexpCollectionPrettyPrinter("zeke_thread")
    pp.add_printer('thread_info', '^thread_info$', thread_infoPrinter)
    return pp


# Commands

class ThreadPrint(gdb.Command):
    "Print thread information"

    def __init__(self):
        super(ThreadPrint, self).__init__("print-thread",
                gdb.COMMAND_DATA, gdb.COMPLETE_NONE, True)

class ThreadTreePrint(gdb.Command):
    "Print thread tree of a given thread"

    def __init__(self):
        super(ThreadTreePrint, self).__init__("print-thread tree", 
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
            return ''

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

ThreadPrint()
ThreadTreePrint()
