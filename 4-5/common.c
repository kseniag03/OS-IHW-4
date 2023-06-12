#include "common.h"

void handleSigInt(int sig)
{
    if (sig != SIGINT)
    {
        return;
    }
    closeAll();
    exit(0);
}

void dieWithError(char *message)
{
    perror(message);
    exit(1);
}
