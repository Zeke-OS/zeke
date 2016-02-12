import gdb.printing
import zeke_klocks
import zeke_proc
import zeke_px
import zeke_queue
import zeke_signal
import zeke_thread

gdb.printing.register_pretty_printer(
        gdb.current_objfile(),
        zeke_klocks.build_pretty_printer())

gdb.printing.register_pretty_printer(
        gdb.current_objfile(),
        zeke_proc.build_pretty_printer())

gdb.printing.register_pretty_printer(
        gdb.current_objfile(),
        zeke_signal.build_pretty_printer())

gdb.printing.register_pretty_printer(
        gdb.current_objfile(),
        zeke_thread.build_pretty_printer())
