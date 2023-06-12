#include "common.h"
#include <semaphore.h>

#define MAX_TASK_COUNT 4

void printTasksInfo();

int sock;

sem_t sem, print;

void closeAll()
{
    printf("\n\nFINISH SERVER USING SIGINT\n\n");
    printTasksInfo();
    sem_destroy(&sem);
    sem_destroy(&print);
    close(sock);
}

//////////// TASK LOGIC START ////////////

struct task tasks[MAX_TASK_COUNT];
struct task curExecute, curCheck;
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
    sem_wait(&print);
    for (int j = 0; j < tasks_count; ++j)
    {
        printf("task with id = %d and status = %d, executed by programmer #%d, checked by programmer %d\n", tasks[j].id, tasks[j].status, tasks[j].executor_id, tasks[j].checker_id);
    }
    sem_post(&print);
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
            curExecute = tasks[i];
            return;
        }
        else if (tasks[i].status == FIX && tasks[i].executor_id == programmer_id)
        {
            // fix task
            printf("programmer #%d is fixing task with id = %d\n", programmer_id, tasks[i].id);
            response->response_code = NEW_TASK;
            tasks[i].status = EXECUTING;
            curExecute = tasks[i];
            return;
        }
        else if (tasks[i].status == EXECUTED && tasks[i].executor_id != programmer_id)
        {
            // found task for check
            printf("programmer #%d has found task with id = %d for checking\n", programmer_id, tasks[i].id);
            response->response_code = CHECK_TASK;
            tasks[i].checker_id = programmer_id;
            tasks[i].status = CHECKING;
            curCheck = tasks[i];
            return;
        }
        else if (tasks[i].executor_id == programmer_id && tasks[i].status == WRONG)
        {
            // found task for fix
            printf("programmer #%d need to fix his task with id = %d\n", programmer_id, tasks[i].id);
            response->response_code = FIX_TASK;
            tasks[i].status = FIX;
            curExecute = tasks[i];
            return;
        }
        else
        {
            // ub
            // one possible case: programmer is trying to check its own executed task
        }
    }
    // no work found
    // setup finish code if all tasks are completed
    if (complete_count == tasks_count)
    {
        response->response_code = FINISH;
    }
}

void sendTask()
{
    // programmer has executed his task
    curExecute.status = EXECUTED;
    tasks[curExecute.id] = curExecute;
}

void sendCheckResult()
{
    // imitating check
    int8_t result = rand() % 2;
    printf("the result of checking task with id = %d is %d\n", curCheck.id, result);
    curCheck.status = result == 0 ? WRONG : RIGHT;
    tasks[curCheck.id] = curCheck;
    if (curCheck.status == RIGHT)
    {
        ++complete_count;
        printf("\n\n!!!!!!!!\tComplete count = %d\t!!!!!!!!!!\n\n", complete_count);
    }
}

//////////// TASK LOGIC END ////////////

int main(int argc, char *argv[])
{
    (void)signal(SIGINT, handleSigInt);

    struct sockaddr_in servaddr, cliaddr;

    // Создание UDP сокета
    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == -1)
    {
        dieWithError("socket() failed");
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
        dieWithError("bind() failed");
    }

    tasks_count = MAX_TASK_COUNT;
    initPulls();

    struct request request;
    struct response response;

    while (complete_count < tasks_count)
    {
        printf("Server listening...\n");

        // receive request from client
        socklen_t len = sizeof(cliaddr);
        recvfrom(sock, &request, sizeof(struct request), 0, (struct sockaddr *)&cliaddr, &len);

        printf("Request code: %d\n", request.request_code);
        printf("Programmer ID: %d\n", request.programmer_id);
        printf("\n");

        sem_wait(&sem);

        int programmer_id = request.programmer_id;

        switch (request.request_code)
        {
        case 0: // get work
            printf("gone to getwork\n");
            getWork(&response, programmer_id);
            break;
        case 1: // send task
            printf("gone to sendtask\n");
            sendTask();
            getWork(&response, programmer_id);
            break;
        case 2: // send check result
            printf("gone to sendcheck\n");
            sendCheckResult();
            getWork(&response, programmer_id);
            break;
        default: // ub
            printf("ub request code\n");
            break;
        }
        printf("\n");

        sem_post(&sem);

        // send response to client
        sendto(sock, &response, sizeof(struct response), 0, (struct sockaddr *)&cliaddr, len);
    }
    // finish socket work
    printTasksInfo();
    close(sock);
    return 0;
}
