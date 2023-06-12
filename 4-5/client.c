#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <pthread.h>
#include <signal.h>

enum REQUEST_CODE
{
    GET_WORK = 0,
    SEND_TASK = 1,
    SEND_CHECK = 2
};

enum RESPONSE_CODE
{
    UB = -1,
    NEW_TASK = 0,
    CHECK_TASK = 1,
    FIX_TASK = 2,
    FINISH = 3
};

enum STATUS
{
    NEW = -1,
    EXECUTING = 0,
    EXECUTED = 1,
    CHECKING = 2,
    WRONG = 3,
    RIGHT = 4,
    FIX = 5
};

struct task
{
    int id;
    int executor_id;
    int checker_id;
    int status;
};

struct request
{
    int request_code;
    int programmer_id;
    struct task task;
};

struct response
{
    int response_code;
    struct task task;
};

int sock;

void closeAll()
{
    printf("\n\nFINISH CLIENT USING SIGINT\n\n");
    close(sock);
}

void handleSigInt(int sig)
{
    if (sig != SIGINT)
    {
        return;
    }
    closeAll();
    exit(0);
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
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    // set socket to allow broadcast
    broadcastPermission = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (void *)&broadcastPermission, sizeof(broadcastPermission)) < 0)
    {
        perror("setsockopt() failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));

    // Установка адреса и порта сервера
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(7004);
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1"); // IP-адрес сервера

    if (argc < 2)
    {
        exit(1);
    }

    int programmer_id = atoi(argv[1]);

    for (;;)
    {

        struct request request;
        request.request_code = 1;
        request.programmer_id = programmer_id;
        request.task.id = 1;
        request.task.executor_id = 2001;
        request.task.checker_id = 3001;
        request.task.status = 0;

        // Отправка запроса серверу
        sendto(sock, &request, sizeof(struct request), 0, (struct sockaddr *)&servaddr, sizeof(servaddr));

        struct response response;

        // Ожидание ответа от сервера
        recvfrom(sock, &response, sizeof(struct response), 0, NULL, NULL);

        printf("Response code: %d\n", response.response_code);
        printf("Task ID: %d\n", response.task.id);
        printf("Executor ID: %d\n", response.task.executor_id);
        printf("Checker ID: %d\n", response.task.checker_id);
        printf("Status: %d\n", response.task.status);

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
            request.task = response.task;
            request.request_code = SEND_TASK;
            break;
        case CHECK_TASK:
            sleep(1);

            // imitating check
            int8_t result = rand() % 2;
            printf("the result of checking task with id = %d is %d\n", response.task.id, result);
            response.task.status = result == 0 ? WRONG : RIGHT;

            request.task = response.task;
            request.request_code = SEND_CHECK;
            break;
        case FIX_TASK:
            sleep(1);
            request.task = response.task;
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
