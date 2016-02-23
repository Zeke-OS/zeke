int __kinit__ mod_init(void)
{
    SUBSYS_DEP(modx_init); /* Module dependency */
    SUBSYS_INIT("module name");
    ...
    return 0;
}

void mod_deinit(void) __attribute__((destructor))
{ ... }
