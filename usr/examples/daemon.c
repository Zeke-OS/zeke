#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

int main(int argc, char * argv[])
{
    pid_t pid, sid;
    FILE * fp;

    pid = fork();
    if (pid < 0) {
        printf("fork failed!\n");
        exit(1);
    }
    if (pid > 0) {
        printf("pid of child process %d\n", pid);
        exit(0);
    }

    umask(0);
    sid = setsid();
    if (sid < 0) {
        exit(1);
    }

    chdir("/");
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    fp = fopen("/tmp/daemon_log.txt", "w+");

    while (1) {
        fprintf(fp, "Logging info...\n");
        fflush(fp);
        sleep(10);
    }

    fclose(fp);

    return 0;
}
