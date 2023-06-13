#include "common.h"

void handleSigInt(int sig)
{
    if (sig != SIGINT)
    {
        return;
    }
    printf("\n\nFINISH USING SIGINT\n\n");
    closeAll();
    exit(0);
}

void dieWithError(char *message)
{
    perror(message);
    exit(1);
}
