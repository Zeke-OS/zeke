import gdb
import gdb.printing
import string
import tempfile

# Commands

class PrintHex(gdb.Command):
    """px - hexdump of N bytes\n
Usage: px <addr> [size]"""
    
    def __init__(self):
        super(PrintHex, self).__init__("px",
                gdb.COMMAND_DATA, gdb.COMPLETE_SYMBOL, False)

    def invoke(self, arg, from_tty):
        argv = gdb.string_to_argv(arg);
        arg0 = argv[0];
        arg1 = 0x80
        if len(argv) >= 2:
            arg1 = argv[1];
        tf = tempfile.NamedTemporaryFile()
        gdb.execute("dump binary memory " + tf.name + " %s (char *)%s+%s" % 
                (arg0, arg0, arg1))
        gdb.execute("shell hexdump " + tf.name)

# Register commands
PrintHex()
