static inline const char * _signal_signum2str(int signum)
{
    const char * const sys_signames[] = {
        "SIG0",
        "SIGHUP",
        "SIGINT",
        "SIGQUIT",
        "SIGILL",
        "SIGTRAP",
        "SIGABRT",
        "SIGCHLD",
        "SIGFPE",
        "SIGKILL",
        "SIGBUS",
        "SIGSEGV",
        "SIGCONT",
        "SIGPIPE",
        "SIGALRM",
        "SIGTERM",
        "SIGSTOP",
        "SIGTSTP",
        "SIGTTIN",
        "SIGTTOU",
        "SIGUSR1",
        "SIGUSR2",
        "SIGSYS",
        "SIGURG",
        "SIGINFO",
        "SIGPWR",
        "SIGCHLDTHRD",
        "SIGCANCEL",
        "SPARE1",
        "SPARE2",
        "SPARE3",
        "_SIGMTX",
    };

    if (signum < 0 || signum >= (int)(sizeof(sys_signames)))
        return "INVALID";
    return sys_signames[signum];
}
