#include "common.h"
#include <semaphore.h>

#define MAX_TASK_COUNT 10

void printTasksInfo();

int sock;

void closeAll()
{
    printTasksInfo();
    close(sock);
}

//////////// TASK LOGIC START ////////////

struct task tasks[MAX_TASK_COUNT];
struct task for_execute[MAX_TASK_COUNT];
struct task for_check[MAX_TASK_COUNT];
// struct task curExecute;
int tasks_count, complete_count = 0;

void initPulls()
{
    // initialize tasks
    for (int i = 0; i < tasks_count; ++i)
    {
        struct task task = {.id = i, .executor_id = -1, .checker_id = -1, .status = NEW};
        tasks[i] = task;
        for_execute[i] = task;
        for_check[i] = task;
    }
}

void printTasksInfo()
{
    for (int j = 0; j < tasks_count; ++j)
    {
        printf("task with id = %d and status = %d, executed by programmer #%d, checked by programmer %d\n", tasks[j].id, tasks[j].status, tasks[j].executor_id, tasks[j].checker_id);
    }
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
            // curExecute = tasks[i];
            for_execute[tasks[i].id] = tasks[i];
            return;
        }
        else if (tasks[i].status == FIX && tasks[i].executor_id == programmer_id)
        {
            // fix task
            printf("programmer #%d is fixing task with id = %d\n", programmer_id, tasks[i].id);
            response->response_code = NEW_TASK;
            tasks[i].status = EXECUTING;
            // curExecute = tasks[i];
            for_execute[tasks[i].id] = tasks[i];
            return;
        }
        else if (tasks[i].status == EXECUTED && tasks[i].executor_id != programmer_id)
        {
            // found task for check
            printf("programmer #%d has found task with id = %d for checking\n", programmer_id, tasks[i].id);
            response->response_code = CHECK_TASK;
            tasks[i].checker_id = programmer_id;
            tasks[i].status = CHECKING;
            // curCheck = tasks[i];
            for_check[tasks[i].id] = tasks[i];
            return;
        }
        else if (tasks[i].executor_id == programmer_id && tasks[i].status == WRONG)
        {
            // found task for fix
            printf("programmer #%d need to fix his task with id = %d\n", programmer_id, tasks[i].id);
            response->response_code = FIX_TASK;
            tasks[i].status = FIX;
            // curExecute = tasks[i];
            for_execute[tasks[i].id] = tasks[i];
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

void sendTask(struct response *response, int programmer_id)
{
    // find task from execute-list with necessary programmer id
    struct task curExecute = {-1, -1, -1, -1};
    for (int i = 0; i < tasks_count; ++i)
    {
        if (for_execute[i].executor_id == programmer_id && for_execute[i].status == EXECUTING)
        {
            curExecute = for_execute[i];
            break;
        }
    }
    if (curExecute.status != EXECUTING)
    {
        printf("task is not executing now...\n");
        printf("task current status = %d\n", curExecute.status);
        response->response_code = 10;
        return;
    }
    // programmer has executed his task
    curExecute.status = EXECUTED;
    tasks[curExecute.id] = curExecute;
}

void sendCheckResult(struct response *response, int programmer_id)
{
    // find task from check-list with necessary programmer id
    struct task curCheck = {-1, -1, -1, -1};
    for (int i = 0; i < tasks_count; ++i)
    {
        if (for_check[i].checker_id == programmer_id && for_check[i].status == CHECKING)
        {
            curCheck = for_check[i];
            // break;
            if (curCheck.status != CHECKING)
            {
                printf("task is not checking now...\n");
                printf("task current status = %d\n", curCheck.status);
                response->response_code = 10;
                return;
            }
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
    }
}

//////////// TASK LOGIC END ////////////

int main(int argc, char *argv[])
{
    (void)signal(SIGINT, handleSigInt);

    struct sockaddr_in servaddr, cliaddr;
    unsigned short port;

    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <Broadcast Port> [Tasks count]\n", argv[0]);
        exit(1);
    }
    port = atoi(argv[1]);
    tasks_count = MAX_TASK_COUNT;
    if (argc > 2)
    {
        tasks_count = atoi(argv[2]);
        tasks_count = (tasks_count > MAX_TASK_COUNT || tasks_count < 2) ? MAX_TASK_COUNT : tasks_count;
    }

    // create a datagram socket using UDP
    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == -1)
    {
        dieWithError("socket() failed");
    }

    // setup local address structure
    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);

    // bind to the broadcast port
    if (bind(sock, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1)
    {
        dieWithError("bind() failed");
    }

    struct request request;
    struct response response;

    initPulls();
    printf("pulls have been initialized\n");

    while (complete_count < tasks_count)
    {
        printf("Server listening...\n");

        // receive request from client
        socklen_t len = sizeof(cliaddr);
        recvfrom(sock, &request, sizeof(struct request), 0, (struct sockaddr *)&cliaddr, &len);

        printf("Request code: %d\n", request.request_code);
        printf("Programmer ID: %d\n", request.programmer_id);
        printf("\n");

        int programmer_id = request.programmer_id;

        switch (request.request_code)
        {
        case 0: // get work
            printf("gone to getwork\n");
            getWork(&response, programmer_id);
            break;
        case 1: // send task
            printf("gone to sendtask\n");
            sendTask(&response, programmer_id);
            getWork(&response, programmer_id);
            break;
        case 2: // send check result
            printf("gone to sendcheck\n");
            sendCheckResult(&response, programmer_id);
            getWork(&response, programmer_id);
            break;
        default: // ub
            printf("ub request code\n");
            break;
        }
        printf("\n");

        // send response to client
        sendto(sock, &response, sizeof(struct response), 0, (struct sockaddr *)&cliaddr, len);
    }

    // finish socket work
    closeAll();
    return 0;
}
