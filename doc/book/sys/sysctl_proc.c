/**
 * sysctl function to read current klogger and change it.
 */
static int sysctl_kern_klogger(SYSCTL_HANDLER_ARGS)
{
    int error;

    error = sysctl_handle_int(oidp, &curr_klogger, sizeof(curr_klogger), req);
    if (!error && req->newptr) {
        kputs = kputs_arr[curr_klogger];
    }

    return error;
}

SYSCTL_PROC(_kern, OID_AUTO, klogger, CTLTYPE_INT | CTLFLAG_RW,
        NULL, 0, sysctl_kern_klogger, "I", "Kernel logger type.");