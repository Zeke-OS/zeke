import gdb.printing
import zeke_queue
import zeke_proc
import zeke_proc_list

gdb.printing.register_pretty_printer(
        gdb.current_objfile(),
        zeke_proc.build_pretty_printer())
