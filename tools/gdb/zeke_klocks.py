import gdb
import gdb.printing
import string

# Pretty printers

class mtxPrinter:
    "Print mtx_t"

    def __init__(self, val):
        self.val = val

    def to_string(self):
        if self.val.address == 0:
            return 'NULL'

        mtx_type = self.val['mtx_type']
        mtx_flags = self.val['mtx_flags']

        s = '{mtx_type: ' + str(mtx_type) + ', flags: ' + str(mtx_flags) + '}'
        return s

def build_pretty_printer():
    pp = gdb.printing.RegexpCollectionPrettyPrinter("zeke_klocks")
    pp.add_printer('mtx', '^mtx$', mtxPrinter)
    
    return pp
