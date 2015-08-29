import gdb.printing
import zeke_proc
import zeke_proc_list
import zeke_queue
import zeke_thread
import zeke_thread_tree

gdb.printing.register_pretty_printer(
        gdb.current_objfile(),
        zeke_proc.build_pretty_printer())

gdb.printing.register_pretty_printer(
        gdb.current_objfile(),
        zeke_thread.build_pretty_printer())
