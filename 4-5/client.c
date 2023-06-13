#include "common.h"

int sock;

void closeAll()
{
    close(sock);
}

int main(int argc, char *argv[])
{
    (void)signal(SIGINT, handleSigInt);

    struct sockaddr_in servaddr;
    char *ip;                // IP broadcast address
    unsigned short port;     // server port
    int broadcastPermission; // socket opt to set permission to broadcast

    if (argc < 4)
    {
        fprintf(stderr, "Usage: %s <IP Address> <Port> <Programmer ID>\n", argv[0]);
        exit(1);
    }
    ip = argv[1];
    port = atoi(argv[2]);
    int programmer_id = atoi(argv[3]);

    // create socket with UDP options
    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == -1)
    {
        dieWithError("socket() failed");
    }

    // set socket to allow broadcast
    broadcastPermission = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (void *)&broadcastPermission, sizeof(broadcastPermission)) < 0)
    {
        dieWithError("setsockopt() failed");
    }

    // setup local address structure
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    servaddr.sin_addr.s_addr = inet_addr(ip);

    struct request request;
    request.request_code = GET_WORK;
    request.programmer_id = programmer_id;
    struct response response;

    for (;;)
    {
        // send request to server
        if (sendto(sock, &request, sizeof(struct request), 0, (struct sockaddr *)&servaddr, sizeof(servaddr)) != sizeof(request))
        {
            dieWithError("sendto() failed");
        }
        printf("programmer#%d has sent request with code = %d\n", request.programmer_id, request.request_code);

        // receive response from server
        if (recvfrom(sock, &response, sizeof(struct response), 0, NULL, NULL) != sizeof(response))
        {
            dieWithError("recvfrom() failed");
        }
        printf("response code: %d\n", response.response_code);

        if (response.response_code == FINISH)
        {
            break;
        }
        // change request depending on response
        switch (response.response_code)
        {
        case UB:
            break;
        case NEW_TASK:
            sleep(3);
            request.request_code = SEND_TASK;
            break;
        case CHECK_TASK:
            sleep(3);
            request.request_code = SEND_CHECK;
            break;
        case FIX_TASK:
            sleep(3);
            request.request_code = SEND_TASK;
            break;
        case FINISH:
            close(sock);
            exit(0);
        default:
            request.request_code = GET_WORK;
            break;
        }
        sleep(5);
    }

    // close socket
    close(sock);
    exit(0);
}
