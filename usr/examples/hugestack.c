#include <stdio.h>
#include <sys/elf_notes.h>

ELFNOTE_STACKSIZE(16384);

void recurse(int level)
{
    char str[1024];

    sprintf(str, "r %d", level);
    puts(str);

    if (level < 10)
        recurse(level + 1);
}

int main(void)
{
    recurse(1);

    return 0;
}
