RCU - Read Copy Update
======================

<span data-acronym-label="rcu" data-acronym-form="singular+full">rcu</span>
is a low overhead synchronization method mainly used for synchronization
between multiple readers and rarely occuring writers.
<span data-acronym-label="rcu" data-acronym-form="singular+abbrv">rcu</span>
can be considered to be a lightweight versioning system for data
structures that allows readers to access the old version of the data as
long as they hold a reference to the old data, while new readers will
get a pointer to the new version.

The
<span data-acronym-label="rcu" data-acronym-form="singular+abbrv">rcu</span>
implementation supports only single writer and multiple readers by
itself but the user may use other synchronization methods to allow
multiple writers. Unlike traditional implementations the
<span data-acronym-label="rcu" data-acronym-form="singular+abbrv">rcu</span>
implentation in Zeke allows interrupts and sleeping on both sides.
Instead of relying on time-based grace periods the
<span data-acronym-label="rcu" data-acronym-form="singular+abbrv">rcu</span>
state is changed only when necessary due to a write synchronization. The
implementation also supports timer based reclamation of resources
similar to the traditional callback API. In practice the algorithm is
based on atomically accessed clock variable and two counters.

Before going any further with describing the implementation, let’s
define the terminology and function names used in this chapter

  - `gptr` is the global pointer referenced by both, readers and
    writer(s),

  - `rcu_assing_pointer()` writes a new value to a `gptr`,

  - `rcu_dereference_pointer()` dereferences the value of a `gptr`,

  - `rcu_read_lock()` increments the number of readers counter,

  - `rcu_read_unlock()` decrements the number of readers counter,

  - `rcu_call()` register a callback to be called when all readers of
    the old version of a `gptr` are ready, and

  - `rcu_synchronize()` block until all current readers are ready.

The global control variables for
<span data-acronym-label="rcu" data-acronym-form="singular+abbrv">rcu</span>
are defined as follows

  - `clk` is a one bit clock selecting the
    <span data-acronym-label="rcu" data-acronym-form="singular+abbrv">rcu</span>
    state,

  - `ctr0` is a counter for readers accepted in the first state,

  - `ctr1` is a counter for readers accepted in the second state, and

  - `rcu_reclaim_list[2]` contains a list of callbacks per state to old
    the versions of the resources.

In the following description we assume that the
<span data-acronym-label="rcu" data-acronym-form="singular+abbrv">rcu</span>
is in the same state as initially but it has already ran an undefined
number of iterations, therefore \(\mathtt{clk} = 0\).

Both states work equally and the clock just swaps the functionality. In
the initial state `ctr0` is incremented on every call to
`rcu_read_lock()`. Whereas `rcu_read_unlock()` always decrements the
same counter that the previous call to `rcu_read_lock()` incremented,
this is ensured by passing a state variable to the caller when
`rcu_read_lock()` returns. After acquiring the lock a reader may call
`rcu_dereference_pointer()` to get the value pointed by a `gptr`.

The writer may call `rcu_assing_pointer()` to assing a new value to the
`gptr`. After the pointer has been assigned the writer should call
either `rcu_call()` or  
`rcu_synchronize()` to ensure there is no more readers for the old data,
and to free the old data. In general the latter is preferred if the
writer is allowed to block.

`rcu_synchronize()` is the only mechanism that will advance the global
<span data-acronym-label="rcu" data-acronym-form="singular+abbrv">rcu</span>
clock variable. The function works in two stages and access to the
function is syncronized by a spinlock. In the first stage it waits until
there is no more readers on the second counter (`ctr1`), and changes the
clock state. The second stage waits until there is no more readers on
the counter pointed by the previous clock state, in the initial case
this means `ctr0`. When `ctr0` has reached zero the reclamation of
resources registered to the reclaim list of the first state can be
executed. Note that these resources are not the ones that were assigned
to `gptr` pointers when the clock was in zero state but the resources
that were alloced when the clock was previously in the current state set
in the previous stage of `rcu_synchronize()`.
