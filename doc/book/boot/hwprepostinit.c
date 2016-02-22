void mod_preinit(void)
{
    SUBSYS_INIT();
    ...
    SUBSYS_INITFINI("mod preinit OK");
}
HW_PREINIT_ENTRY(mod_preinit);

void mod_postinit(void)
{ ... }
HW_POSTINIT_ENTRY(mod_postinit);
