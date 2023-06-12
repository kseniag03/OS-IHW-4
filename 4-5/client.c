#include "common.h"

int sock;

void closeAll()
{
    printf("\n\nFINISH CLIENT USING SIGINT\n\n");
    close(sock);
}

int main(int argc, char *argv[])
{
    (void)signal(SIGINT, handleSigInt);

    struct sockaddr_in servaddr;
    int broadcastPermission; // socket opt to set permission to broadcast

    // Создание UDP сокета
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

    memset(&servaddr, 0, sizeof(servaddr));

    // Установка адреса и порта сервера
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(7004);
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1"); // IP-адрес сервера

    if (argc < 2)
    {
        dieWithError("invalid arguments");
    }

    int programmer_id = atoi(argv[1]);

    struct request request;
    request.request_code = GET_WORK;
    request.programmer_id = programmer_id;

    for (;;)
    {

        /*
        request.task.id = NEW;
        request.task.executor_id = -1;
        request.task.checker_id = -1;
        request.task.status = -1;*/

        // Отправка запроса серверу

        printf("programmer#%d has sent request with code = %d\n", request.programmer_id, request.request_code);

        sendto(sock, &request, sizeof(struct request), 0, (struct sockaddr *)&servaddr, sizeof(servaddr));

        struct response response;

        // Ожидание ответа от сервера
        recvfrom(sock, &response, sizeof(struct response), 0, NULL, NULL);

        printf("Response code: %d\n", response.response_code);
        /*
        printf("Task ID: %d\n", response.task.id);
        printf("Executor ID: %d\n", response.task.executor_id);
        printf("Checker ID: %d\n", response.task.checker_id);
        printf("Status: %d\n", response.task.status);*/

        if (response.response_code == FINISH)
        {
            break;
        }

        switch (response.response_code)
        {
        case UB:
            break;
        case NEW_TASK:
            sleep(1);
            // request.task = response.task;
            request.request_code = SEND_TASK;
            break;
        case CHECK_TASK:
            sleep(1);
            request.request_code = SEND_CHECK;
            break;
        case FIX_TASK:
            sleep(1);
            // request.task = response.task;
            request.request_code = SEND_TASK;
            break;
        default:
            request.request_code = GET_WORK;
            break;
        }

        sleep(3);
    }

    // Закрытие сокета
    close(sock);

    return 0;
}
