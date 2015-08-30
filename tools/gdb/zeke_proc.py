import gdb
import gdb.printing
import string
import zeke_queue

# Pretty printers

class sessionPrinter:
    "Show session information"

    def __init__(self, val):
        self.val = val

    def to_string(self):
        if self.val.address == 0:
            return "NULL"

        s_leader = self.val['s_leader']
        s_login = self.val['s_login']
        s_refcount = self.val['s_refcount']
        pgrp_list = zeke_queue.getTAILQ(self.val['s_pgrp_list_head'], 'pg_pgrp_entry_')

        session = '{Session leader: ' + str(s_leader) + \
            ', login: '     + str(s_login) + \
            ', refcount: '  + str(s_refcount) + \
            ', pgroups: '   + str(pgrp_list) + '}'

        return session

class pgrpPrinter:
    "Show process group information"

    def __init__(self, val):
        self.val = val

    def to_string(self):
        if self.val.address == 0:
            return "NULL"
        
        pg_id = self.val['pg_id']
        pg_sid = self.val['pg_session'].dereference()['s_leader']
        pg_refcount = self.val['pg_refcount']

        pgrp = '{pg_id: '   + str(self.val['pg_id']) + \
            ', sid: '       + str(pg_sid) + \
            ', refcount: '  + str(pg_refcount) + '}'

        return pgrp

class proc_infoPrinter:
    "Show process information"

    def __init__(self, val):
        self.val = val

    def to_string(self):
        if self.val.address == 0:
            return "NULL"

        p_pid = self.val['pid']
        p_name = str(self.val['name']).split('\\000', 1)[0] + '"'
        p_state = self.val['state']
        p_priority = self.val['priority']
        p_exit_code = self.val['exit_code']
        p_exit_signal = self.val['exit_signal']
        p_parent = self.val['inh']['parent'].dereference()['pid']
        p_main_thread = self.val['main_thread'].dereference()['id']
        p_pgrp = self.val['pgrp'].dereference()['pg_id']

        proc  = '{PID: '    + str(p_pid) + \
            ', name: '      + p_name + \
            ', state: '     + str(p_state) + \
            ', priority: '  + str(p_priority) + \
            ', exit_c/s: '  + str(p_exit_code) + '/' + str(p_exit_signal) + \
            ', parent: '    + str(p_parent) + \
            ', main thread: ' + str(p_main_thread) + \
            ', pgrp: '      + str(p_pgrp) + '}'

        return proc


def build_pretty_printer():
    pp = gdb.printing.RegexpCollectionPrettyPrinter("zeke_proc")
    pp.add_printer('session', '^session$', sessionPrinter)
    pp.add_printer('pgrp', '^pgrp$', pgrpPrinter)
    pp.add_printer('proc_info', '^proc_info$', proc_infoPrinter)
    return pp


# Commands

class ProcPrint(gdb.Command):
    "Print process information"

    def __init__(self):
        super(ProcPrint, self).__init__("print-proc",
                gdb.COMMAND_DATA, gdb.COMPLETE_NONE, True)

class ProcPidPrint(gdb.Command):
    "Print process by PID"

    def __init__(self):
        super(ProcPidPrint, self).__init__("print-proc pid",
                gdb.COMMAND_DATA)

    def invoke(self, arg, from_tty):
        try:
            pid = gdb.string_to_argv(arg)[0]
            proc = gdb.parse_and_eval('*(*_procarr)[' + str(pid) + ']')
            print '((struct proc_info *)' +  str(proc.address) + ')' + str(proc)
        except:
            gdb.write('Invalid PID\n')
            return

class ProcListPrint(gdb.Command):
    "Print process list"

    def __init__(self):
        super(ProcListPrint, self).__init__("print-proc list", 
                gdb.COMMAND_DATA)

    def invoke(self, arg, from_tty):
        try:
            proc_arr = gdb.parse_and_eval('*_procarr')
            act_maxproc = gdb.parse_and_eval('act_maxproc')
        except:
            gdb.write('Failed to deref proc_arr\n')
            return

        for i in range(0, act_maxproc + 1):
            p = proc_arr[i]
            if p != 0:
                print(p.dereference())

class ProcTreePrint(gdb.Command):
    """Print process trees\n
print-proc tree             Prints the tree of all processes in the system.
print-proc tree <proc_info> Prints children of the process.
    """
    def __init__(self):
        super(ProcTreePrint, self).__init__("print-proc tree", 
                gdb.COMMAND_DATA, gdb.COMPLETE_EXPRESSION)

    def invoke(self, arg, from_tty):
        args = gdb.string_to_argv(arg)
        
        if len(args) != 0:
            proc = gdb.parse_and_eval(args[0]).dereference()
        else:
            proc = proc_arr = gdb.parse_and_eval('*_procarr')[0].dereference()

        print ProcTreeToString(proc, 1)

def ProcTreeToString(proc, d):
    if proc.address == 0:
        return '';

    s = '-' + str(proc['pid']) + '\n'

    child = proc['inh']['child_list_head']['slh_first'].dereference()
    while child.address != 0:
        s += d * ' ' + '|'
        s += ProcTreeToString(child, d + 2)
        child = child['inh']['child_list_entry']['sle_next'].dereference()

    return s


# Register commands
ProcPrint()
ProcPidPrint()
ProcTreePrint()
ProcListPrint()
