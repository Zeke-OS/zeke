Debugging with GDB
==================

Add -g to CCFLAGS in genconfig/buildconf.mk.

Get PID and TID
---------------

print current_process_id
print current_thread.id

Set PC and SP if kernel crashed during DAB
------------------------------------------

set $pc = 0xYYY
set $sp = 0xXXX

Get thread stack before panic in DAB
------------------------------------

print/x current_thread.kstack_region.b_data
$2 = 0x403000
set $sp = 0x403f28 # PC should have been printed to the console
set $pc = 0x0002311c
