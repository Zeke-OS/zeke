file kernel.elf
target remote localhost:1234

python
import sys
sys.path.insert(0, 'tools/gdb')
import zeke
end
