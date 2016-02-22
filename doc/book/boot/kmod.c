void mod_init(void) __attribute__((constructor));
void mod_deinit(void) __attribute__((destructor));

void mod_init(void)
{
    SUBSYS_INIT();
    SUBSYS_DEP(modx_init); /* Module dependency */
    ...
    SUBSYS_INITFINI("mod OK");
}

void mod_deinit(void)
{ ... }
