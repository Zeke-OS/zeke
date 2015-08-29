import gdb
import gdb.printing
import string
import zeke_queue

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
