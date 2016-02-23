int mod_preinit(void)
{
    SUBSYS_INIT("hw driver");
    ...
    return 0;
}
HW_PREINIT_ENTRY(mod_preinit);

int mod_postinit(void)
{
    SUBSYS_INIT("hw driver");
    ...
    return 0;
}
HW_POSTINIT_ENTRY(mod_postinit);
