import gdb
import gdb.printing
import string

# Pretty printers

class __sigsetPrinter:
    "Print __sigset_t"

    def __init__(self, val):
        self.val = val

    def to_string(self):
        if self.val.address == 0:
            return 'NULL'

        bits = self.val['__bits']
        bsa = [hex(int(bits[0]))[2:].zfill(8),
                hex(int(bits[1]))[2:].zfill(8),
                hex(int(bits[2]))[2:].zfill(8),
                hex(int(bits[3]))[2:].zfill(8)]
        s = '0x' + bsa[3] + bsa[2] + bsa[1] + bsa[0]
        return s


def build_pretty_printer():
    pp = gdb.printing.RegexpCollectionPrettyPrinter("zeke_signal")
    pp.add_printer('__sigset', '^__sigset$', __sigsetPrinter)
    
    return pp
