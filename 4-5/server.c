#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <pthread.h>
#include <signal.h>

#define MAX_TASK_COUNT 10

void printTasksInfo();

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
    printf("\n\nFINISH SERVER USING SIGINT\n\n");
    printTasksInfo();
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

//////////// TASK LOGIC

struct task tasks[MAX_TASK_COUNT];
int tasks_count, complete_count = 0;

void initPulls()
{
    // initialize tasks
    for (int i = 0; i < tasks_count; ++i)
    {
        struct task task = {.id = i, .executor_id = -1, .checker_id = -1, .status = -1};
        tasks[i] = task;
    }
}

void printTasksInfo()
{
    // sem_wait(&print);
    for (int j = 0; j < tasks_count; ++j)
    {
        printf("task with id = %d and status = %d, executed by programmer #%d, checked by programmer %d\n", tasks[j].id, tasks[j].status, tasks[j].executor_id, tasks[j].checker_id);
    }
    // sem_post(&print);
}

void getWork(struct response *response, int programmer_id)
{
    for (int i = 0; i < tasks_count; ++i)
    {
        if (tasks[i].status == NEW)
        {
            // found task for execution
            printf("programmer #%d has found task with id = %d for executing\n", programmer_id, tasks[i].id);
            response->response_code = NEW_TASK;
            tasks[i].executor_id = programmer_id;
            tasks[i].status = EXECUTING;
            response->task = tasks[i];
            return;
        }
        else if (tasks[i].status == FIX && tasks[i].executor_id == programmer_id)
        {
            // fix task
            printf("programmer #%d is fixing task with id = %d\n", programmer_id, tasks[i].id);
            response->response_code = NEW_TASK;
            tasks[i].status = EXECUTING;
            response->task = tasks[i];
            return;
        }
        else if (tasks[i].status == EXECUTED && tasks[i].executor_id != programmer_id)
        {
            // found task for check
            printf("programmer #%d has found task with id = %d for checking\n", programmer_id, tasks[i].id);
            response->response_code = CHECK_TASK;
            tasks[i].checker_id = programmer_id;
            tasks[i].status = CHECKING;
            response->task = tasks[i];
            return;
        }
        else if (tasks[i].executor_id == programmer_id && tasks[i].status == WRONG)
        {
            // found task for fix
            printf("programmer #%d need to fix his task with id = %d\n", programmer_id, tasks[i].id);
            response->response_code = FIX_TASK;
            tasks[i].status = FIX;
            response->task = tasks[i];
            return;
        }
        else
        {
            // ub?
            // one possible case: programmer is trying to check its own executed task
        }
    }
    // no work found

    if (complete_count == tasks_count)
    {
        response->response_code = FINISH;
    }
}

void sendTask(struct task *task)
{
    // programmer has executed his task
    task->status = EXECUTED;
    tasks[task->id] = *task;
}

void sendCheckResult(struct task *task)
{
    tasks[task->id] = *task;
    if (task->status == RIGHT)
    {
        ++complete_count;
        printf("\n\n!!!!!!!!\tComplete count = %d\t!!!!!!!!!!\n\n", complete_count);
    }
}

////////////

int main(int argc, char *argv[])
{
    (void)signal(SIGINT, handleSigInt);

    int sock;
    struct sockaddr_in servaddr, cliaddr;

    // Создание UDP сокета
    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == -1)
    {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));

    // Установка адреса и порта сервера
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(7004);

    // Привязка сокета к адресу и порту сервера
    if (bind(sock, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    initPulls();

    struct request request;
    struct response response;

    tasks_count = MAX_TASK_COUNT;

    while (complete_count < tasks_count)
    {

        printf("Server listening...\n");

        // Ожидание запросов от клиента
        socklen_t len = sizeof(cliaddr);
        recvfrom(sock, &request, sizeof(struct request), 0, (struct sockaddr *)&cliaddr, &len);

        printf("Request code: %d\n", request.request_code);
        printf("Programmer ID: %d\n", request.programmer_id);
        printf("Task ID: %d\n", request.task.id);
        printf("Executor ID: %d\n", request.task.executor_id);
        printf("Checker ID: %d\n", request.task.checker_id);
        printf("Status: %d\n", request.task.status);

        int programmer_id = request.programmer_id;
        struct task task = request.task;

        switch (request.request_code)
        {
        case 0: // get work
            getWork(&response, programmer_id);
            break;
        case 1: // send task
            sendTask(&task);
            getWork(&response, programmer_id);
            break;
        case 2: // send check result
            sendCheckResult(&task);
            getWork(&response, programmer_id);
            break;
        default: // ub
            printf("ub request code\n");
            break;
        }

        // Обработка запроса и формирование ответа
        // response.response_code = 200;
        // response.task = request.task;

        // Отправка ответа клиенту
        sendto(sock, &response, sizeof(struct response), 0, (struct sockaddr *)&cliaddr, len);
    }

    // Закрытие сокета
    close(sock);

    return 0;
}
