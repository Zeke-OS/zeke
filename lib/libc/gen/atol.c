long atol(char * p)
{
    long n;
    int f;

    n = 0;
    f = 0;
    for (;; p++) {
        switch (*p) {
        case ' ':
        case '\t':
            continue;
        case '-':
            f++;
        case '+':
            p++;
        }
        break;
    }
    while (*p >= '0' && *p <= '9') {
        n = n * 10 + *p++ - '0';
    }

    return f ? -n : n;
}
