import gdb
import gdb.printing
import string

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
