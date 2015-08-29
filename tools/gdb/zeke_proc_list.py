import gdb

class procListPrint(gdb.Command):
    "Print process list"

    def __init__(self):
        super(procListPrint, self).__init__("print-proc-list", gdb.COMMAND_DATA)

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


procListPrint()
