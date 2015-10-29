#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef enum {false, true} bool; //declaring boolean

//Place all structs here
struct Job
{
    int A;                      //start time of Job
    int B;                      //upper bound
    int C;                      //total CPU time required
    int M;                      //multiplier for IO burst

    int currentWaitTime;        //amount of time waiting in ready queue, increment only in ready queue
    int currentStatus;          //0 is uncreated, 1 is ready, 2 is running, 3 is blocked, 4 is terminated
    int currentCPUTime;         //amount of CPU time already run, increments only in the running mode
    int currentIOBlocked;       //amount of time IO blocked, increments only in blocked list
    int jobID;                  //order that the job came in
    int finishTime;             //marks when the job finishes running
    int currentIOBurst;         //tells how many more cycles to wait in blocked mode
    int currentCPUBurst;        //cannot move from ready to running, until CPUBurst is greater than 0
    int quantum;                //quantum value used in round robin
    bool isFirstTimeRunning;    //whether or not is running for first time

    struct Job* nextReady;      //next job in ready queue
    struct Job* nextReadySuspended; //next job in ready suspended queue
}; //end of Job struct

//global counters
int TOTAL_NUMBER_OF_FINISHED_JOBS = 0;
int TOTAL_CREATED_JOBS = 0;
int CURRENT_TIME_CYCLE = 0;
int TOTAL_STARTED_JOBS = 0;
double TOTAL_NUMBER_OF_BLOCKED_CYCLES = 0.0;
struct Job* RUNNING_UNIPROGRAMMED_JOB;
struct Job* CURRENT_RUNNING_PROCESS;

//global flags
bool IS_VERBOSE;
bool SHOW_RANDOM;

//ready queue pointers
struct Job* readyQueueHead = NULL;
struct Job* readyQueueTail = NULL;
int readyQueueSize = 0;

//readySuspended queue pointers

struct Job* readySuspendedQueueHead = NULL;
struct Job* readySuspendedQueueTail = NULL;
int readySuspendedQueueSize = 0;

//readySuspended queue functions

void enqueueToReadySuspendedQueue(struct Job* jobToBeEnqueued)
{
    if(readySuspendedQueueSize == 0)
    {
        readySuspendedQueueHead = jobToBeEnqueued;
        readySuspendedQueueTail = jobToBeEnqueued;
    }
    else
    {
        readySuspendedQueueTail->nextReadySuspended = jobToBeEnqueued;
        readySuspendedQueueTail = jobToBeEnqueued;
    }
    readySuspendedQueueSize++;
} //end of enqueue to readySuspended function

struct Job* dequeueFromReadySuspendedQueue()
{
    if(readySuspendedQueueSize == 0)
        return NULL;
    struct Job* oldReadySuspendedHead = readySuspendedQueueHead;
    readySuspendedQueueHead = readySuspendedQueueHead->nextReadySuspended;
    oldReadySuspendedHead->nextReadySuspended = NULL;
    readySuspendedQueueSize--;
    if(readySuspendedQueueSize == 0)
        readySuspendedQueueTail = NULL;
    return oldReadySuspendedHead;

} //end of dequeue from readySuspended queue

//ready queue functions
void enqueueToReadyQueue(struct Job* jobToBeEnqueued)
{
    if(readyQueueSize == 0)
    {
        readyQueueHead = jobToBeEnqueued;
        readyQueueTail = jobToBeEnqueued;
    }
    else
    {
        readyQueueTail->nextReady = jobToBeEnqueued;
        readyQueueTail = jobToBeEnqueued;
    }
    readyQueueSize++;
} //end of enqueue to ready function

struct Job* dequeueFromReadyQueue()
{
    if(readyQueueSize == 0)
        return NULL;
    struct Job* oldReadyHead = readyQueueHead;
    readyQueueHead = readyQueueHead->nextReady;
    oldReadyHead->nextReady = NULL;
    readyQueueSize--;
    if(readyQueueSize == 0)
        readyQueueTail = NULL;
    return oldReadyHead;
} //end of dequeue from ready queue

void processBlockedJobs(struct Job jobContainer[])
{
    printf("got to process blocked jobs\n");
    int i;
    for(i = 0; i < TOTAL_CREATED_JOBS; i++)
    {
        struct Job* currentJob = &jobContainer[i];
        if((currentJob->currentStatus == 3) && (currentJob->currentIOBurst <= 0))
        {
            printf("Inside of processblockedJobs and is about to enqueue\n");
            currentJob->currentStatus = 1;
            enqueueToReadyQueue(currentJob);
        }
    }
} //end of processBlockedJobs function

void processRunningJobs(struct Job jobContainer[], struct Job terminatedJobContainer[], int schedulingAlgorithm)
{
    printf("got to process running jobs\n");
    if(CURRENT_RUNNING_PROCESS != NULL)
    {
        if(CURRENT_RUNNING_PROCESS->isFirstTimeRunning)
        {
            CURRENT_RUNNING_PROCESS->isFirstTimeRunning = false;
            CURRENT_RUNNING_PROCESS->currentIOBurst = 1 + (CURRENT_RUNNING_PROCESS->M * CURRENT_RUNNING_PROCESS->currentCPUBurst);
        }

        if(CURRENT_RUNNING_PROCESS->currentCPUTime == CURRENT_RUNNING_PROCESS->C)
        {
            //current running process is now terminated
            CURRENT_RUNNING_PROCESS->currentStatus = 4;
            CURRENT_RUNNING_PROCESS->finishTime = CURRENT_TIME_CYCLE;
            terminatedJobContainer[TOTAL_NUMBER_OF_FINISHED_JOBS] = *CURRENT_RUNNING_PROCESS;
            TOTAL_NUMBER_OF_FINISHED_JOBS++;
            CURRENT_RUNNING_PROCESS = NULL;
            RUNNING_UNIPROGRAMMED_JOB = NULL;
        }
        else if(CURRENT_RUNNING_PROCESS->currentCPUBurst <= 0)
        {
            //current running process is now blocked
            CURRENT_RUNNING_PROCESS->currentStatus = 3;
            CURRENT_RUNNING_PROCESS = NULL;
        }
        else if((schedulingAlgorithm == 1) && (CURRENT_RUNNING_PROCESS->quantum <= 0))
        {
            //current running process is ready because of pre-emption
            CURRENT_RUNNING_PROCESS->currentStatus = 1;
            enqueueToReadyQueue(CURRENT_RUNNING_PROCESS);
            CURRENT_RUNNING_PROCESS = NULL;
        }
        else
        {
            //current running process is still running
            CURRENT_RUNNING_PROCESS-> currentStatus = 2;
        }
    }

} //end of processRunningJobs function

void createJobs(struct Job jobContainer[])
{
    printf("got to createjobs\n");
    int i = 0;
    for(; i < TOTAL_CREATED_JOBS; i++)
    {
        if(jobContainer[i].A == CURRENT_TIME_CYCLE)
        {
            TOTAL_STARTED_JOBS++;
            jobContainer[i].currentStatus = 1;
            if(i == 0)
                RUNNING_UNIPROGRAMMED_JOB = &jobContainer[i];
            struct Job* jobToBeEnqueued = &jobContainer[i];
            enqueueToReadyQueue(jobToBeEnqueued);
        }
    }
} //end of createJobs function

void processReadyJobs(int schedulerAlgorithm, FILE* randomNumberFile)
{
    printf("got to processreadyjobs\n");

    if((readySuspendedQueueSize > 0) && (schedulerAlgorithm == 2) && (RUNNING_UNIPROGRAMMED_JOB == NULL))
    {
        //move from ready suspended to ready
        struct Job* resumedJob = dequeueFromReadySuspendedQueue();
        RUNNING_UNIPROGRAMMED_JOB = resumedJob;
        enqueueToReadyQueue(resumedJob);
    }

    if(readyQueueSize > 0)
    {
        //this is the case for uni programmed
        if((RUNNING_UNIPROGRAMMED_JOB != NULL) && (schedulerAlgorithm == 2)
           && (RUNNING_UNIPROGRAMMED_JOB != readyQueueHead))
        {
            //iterate through readyQueue, suspend all which are not UNI
            int i = 0;
            for(; i < readyQueueSize; ++i)
                enqueueToReadySuspendedQueue(dequeueFromReadyQueue());
        }
        else
        {

            //deals with all other schedulers, and UNI when readyQueueHead == RUNNING_UNI
            if (schedulerAlgorithm == 3)
            {
                //deals with Shortest Job First
                //search through readyQueue for shortest job
                struct Job* current = readyQueueHead;
                struct Job* shortestJobFound = readyQueueHead;
                int i = 0;
                for(; i < readyQueueSize; i++)
                {
                    if((shortestJobFound->C - shortestJobFound->currentCPUTime) > (current->C - current->currentCPUTime))
                        shortestJobFound = current;
                    current = current->nextReady;
                } //at the end, Shortest Job Found is pointing at a job in the ready queue


                //remove from readyQueue the shortest job to be run
                if(shortestJobFound == readyQueueHead)
                {
                    //remove from head, normal
                    dequeueFromReadyQueue();
                }
                else if(shortestJobFound == readyQueueTail)
                {
                    //remove from tail
                    struct Job* currentJob = readyQueueHead;
                    //iterate through the queue
                    while(currentJob->nextReady->nextReady != NULL)
                        currentJob = currentJob->nextReady;

                    currentJob->nextReady = currentJob->nextReady->nextReady;
                    readyQueueTail = currentJob;
                    --readyQueueSize;

                }
                else
                {
                    //remove from middle
                    struct Job* currentJob = readyQueueHead;
                    while(shortestJobFound != currentJob->nextReady)
                        currentJob = currentJob->nextReady;

                    currentJob->nextReady = currentJob->nextReady->nextReady;
                    --readyQueueSize;
                }
                shortestJobFound->nextReady = NULL;
                shortestJobFound->currentStatus = 2;
                shortestJobFound->quantum = 2;
                shortestJobFound->isFirstTimeRunning = true;
                //calculate CPU Burst stuff
                char buffer[15];
                fgets(buffer, 15, randomNumberFile);
                int randomNumber = atoi(buffer);
                int newCPUBurst = 1 + (randomNumber % shortestJobFound->B);
                if(newCPUBurst > shortestJobFound->C - shortestJobFound->currentCPUTime)
                    newCPUBurst = shortestJobFound->C - shortestJobFound->currentCPUTime;

                shortestJobFound->currentCPUBurst = newCPUBurst;

                CURRENT_RUNNING_PROCESS = shortestJobFound;

            } //end of dealing with SJF
            else
            {
                //actions to take if running FCFS or RR or Uni
                if (CURRENT_RUNNING_PROCESS == NULL)
                {
                    struct Job* jobToBeRun = dequeueFromReadyQueue();
                    char buffer[15];
                    fgets(buffer, 15, randomNumberFile);
                    int randomNumber = atoi(buffer);
                    int newCPUBurst = 1 + (randomNumber % jobToBeRun->B);
                    if(newCPUBurst > jobToBeRun->C - jobToBeRun->currentCPUTime)
                        newCPUBurst = jobToBeRun->C - jobToBeRun->currentCPUTime;

                    jobToBeRun->currentCPUBurst = newCPUBurst;
                    jobToBeRun->isFirstTimeRunning = true;
                    jobToBeRun->currentStatus = 2;
                    jobToBeRun->quantum = 2;
                    CURRENT_RUNNING_PROCESS = jobToBeRun;
                }
            } //end of dealing with all other schedulers
        } //end of dealing with all schedulers
    } //finished with ready queue

    if(schedulerAlgorithm == 2 && readyQueueSize > 0)
    {
        int i = 0;
        for(; i < readyQueueSize; i++)
        {
            if(CURRENT_RUNNING_PROCESS != NULL)
                enqueueToReadySuspendedQueue(dequeueFromReadyQueue());
        }
    }


} //end of processReadyJobs

void updateAttributes(struct Job jobContainer[])
{
    printf("got to update attribute\n");
    int i = 0;
    for(; i < TOTAL_CREATED_JOBS; i++)
    {

        if(jobContainer[i].currentStatus == 2)
        {
            //at running stage
            ++jobContainer[i].currentCPUTime;
            --jobContainer[i].currentCPUBurst;
            --jobContainer[i].quantum;
        }
        else if(jobContainer[i].currentStatus == 1)
        {
            //at ready stage
            ++jobContainer[i].currentWaitTime;
        }
        else if(jobContainer[i].currentStatus == 3)
        {
            //at blocked stage
            --jobContainer[i].currentIOBurst;
            ++jobContainer[i].currentIOBlocked;
        }

    }

    //checks if any jobs are blocked
    for(i = 0; i < TOTAL_CREATED_JOBS; i++)
    {
        if(jobContainer[i].currentStatus == 3)
        {
            TOTAL_NUMBER_OF_BLOCKED_CYCLES += 1.0;
            break;
        }
    }

} //end of update attributes function

void simulateRunningJobs(struct Job jobContainer[], struct Job terminatedJobContainer[],
                         int schedulerAlgorithm, FILE* randomFile)
{
    //verbose check
    if(IS_VERBOSE)
    {
        printf("Before cycle\t%i:",CURRENT_TIME_CYCLE);
        //print job status
        int i = 0;
        for(; i < TOTAL_CREATED_JOBS; ++i)
        {
            struct Job currentJob = jobContainer[i];
            if(currentJob.currentStatus == 0)
            {
                //process is not created yet
                printf("\tunstarted\t0.");
            }
            else if(currentJob.currentStatus == 1)
            {
                //process is ready
                printf("\tready\t1.");
            }
            else if(currentJob.currentStatus == 2)
            {
                //process is running
                printf("\trunning\t%i.", currentJob.currentCPUBurst + 1);
            }
            else if(currentJob.currentStatus == 3)
            {
                //process is blocked
                printf("\tblocked\t%i.", currentJob.currentIOBurst + 1);
            }
            else
                printf("\tterminated\t0.");
        }
        printf("\n");
    }

    //process blockedJobs
    processBlockedJobs(jobContainer);
    //process runningJobs
    processRunningJobs(jobContainer, terminatedJobContainer, schedulerAlgorithm);

    //check if jobs have been started, if not, create jobs
    if(TOTAL_STARTED_JOBS < TOTAL_CREATED_JOBS)
        createJobs(jobContainer);


    //process readyJobs
    processReadyJobs(schedulerAlgorithm, randomFile);
    //update attributes
    updateAttributes(jobContainer);
    //increments time
    ++CURRENT_TIME_CYCLE;

} //end of simulateRunningJobs

void printContainer(struct Job jobContainer[], int version)
{
    if(version == 0)
        printf("The original input was: %i", TOTAL_CREATED_JOBS);
    else
        printf("The (sorted) input is: %i", TOTAL_CREATED_JOBS);
    int i = 0;
    for (; i < TOTAL_CREATED_JOBS; ++i)
    {
        printf(" ( %i %i %i %i )", jobContainer[i].A, jobContainer[i].B, jobContainer[i].C, jobContainer[i].M);
    }
    printf("\n");
}//end of running printContainer

void runFirstComeFirstServe(struct Job jobContainer[], struct Job terminatedJobContainer[], int schedulerAlgorithm)
{
    FILE* randomFile = fopen("random-numbers", "r");
    while(TOTAL_NUMBER_OF_FINISHED_JOBS < TOTAL_CREATED_JOBS)
    {
//        if (CURRENT_TIME_CYCLE == 10)
//            break;
        //run scheduler specifics
        simulateRunningJobs(jobContainer, terminatedJobContainer, schedulerAlgorithm, randomFile);
    } //end of simulation
    fclose(randomFile);

} //end of running firstComeFirstServe

void runRoundRobin(struct Job jobContainer[], struct Job terminatedJobContainer[], int schedulerAlgorithm)
{
    FILE* randomFile = fopen("random-numbers", "r");
    while(TOTAL_NUMBER_OF_FINISHED_JOBS < TOTAL_CREATED_JOBS)
    {
//        if (CURRENT_TIME_CYCLE == 10)
//            break;
        //run scheduler specifics
        simulateRunningJobs(jobContainer, terminatedJobContainer, schedulerAlgorithm, randomFile);
    } //end of simulation
    fclose(randomFile);

} //end of running RoundRobin

void runUniProgrammed(struct Job jobContainer[], struct Job terminatedJobContainer[], int schedulerAlgorithm)
{
    FILE* randomFile = fopen("random-numbers", "r");
    while(TOTAL_NUMBER_OF_FINISHED_JOBS < TOTAL_CREATED_JOBS)
    {
//        if (CURRENT_TIME_CYCLE == 10)
//            break;
        //run scheduler specifics
        simulateRunningJobs(jobContainer, terminatedJobContainer, schedulerAlgorithm, randomFile);
    } //end of simulation
    fclose(randomFile);

} //end of running uniProgrammed

void runShortestJobFirst(struct Job jobContainer[], struct Job terminatedJobContainer[], int schedulerAlgorithm)
{
    FILE* randomFile = fopen("random-numbers", "r");
    while(TOTAL_NUMBER_OF_FINISHED_JOBS < TOTAL_CREATED_JOBS)
    {
//        if (CURRENT_TIME_CYCLE == 10)
//            break;
        //run scheduler specifics
        simulateRunningJobs(jobContainer, terminatedJobContainer, schedulerAlgorithm, randomFile);
    } //end of simulation
    fclose(randomFile);

} //end of running shortestJobFirst

void reset(struct Job jobContainer[], struct Job terminatedJobContainer[])
{
//global counters
    TOTAL_NUMBER_OF_FINISHED_JOBS = 0;
    CURRENT_TIME_CYCLE = 0;
    TOTAL_STARTED_JOBS = 0;
    TOTAL_NUMBER_OF_BLOCKED_CYCLES = 0.0;
    RUNNING_UNIPROGRAMMED_JOB = NULL;
    CURRENT_RUNNING_PROCESS = NULL;

//ready queue pointers
    readyQueueHead = NULL;
    readyQueueTail = NULL;
    readyQueueSize = 0;

//readySuspended queue pointers

    readySuspendedQueueHead = NULL;
    readySuspendedQueueTail = NULL;
    readySuspendedQueueSize = 0;
    FILE* randomFile = fopen("random-numbers", "r");
    int i = 0;
    for(; i < TOTAL_CREATED_JOBS; ++i)
    {
        jobContainer[i].currentStatus = 0;
        jobContainer[i].nextReady = NULL;
        jobContainer[i].nextReadySuspended = NULL;
        jobContainer[i].finishTime = 0;
        jobContainer[i].currentCPUTime = 0;
        jobContainer[i].currentIOBlocked = 0;
        jobContainer[i].currentWaitTime = 0;
        jobContainer[i].isFirstTimeRunning = false;
        char buffer[15];
        int randomNumber = atoi(fgets(buffer, 15,randomFile));
        jobContainer[i].currentCPUBurst = 1 + (randomNumber % jobContainer[i].B);
        jobContainer[i].currentIOBurst = jobContainer[i].M * jobContainer[i].currentCPUBurst;
    }
    fclose(randomFile);
} //end of reset

void printPerProcessAttributes(struct Job jobContainer[])
{
    printf("\n");
    int i = 0;
    for(; i <TOTAL_CREATED_JOBS; i++)
    {
        printf("process %i:\n", jobContainer[i].jobID);
        printf("\t(A,B,C,M)= (%i %i %i %i)\n",jobContainer[i].A, jobContainer[i].B, jobContainer[i].C, jobContainer[i].M);
        printf("\tfinishing time: %i\n", jobContainer[i].finishTime);
        printf("\tturnaround time: %i\n", jobContainer[i].finishTime - jobContainer[i].A);
        printf("\tIO time: %i\n", jobContainer[i].currentIOBlocked);
        printf("\twaiting time: %i\n", jobContainer[i].currentWaitTime);
        printf("\n");
    }
} //end of per process attributes printing function

void printSummary(struct Job jobContainer[])
{
    double totalTimeUsingCPU = 0.000000;
    double totalTimeSpentWaiting = 0.000000;
    double totalTurnAroundTime = 0.000000;

    int i = 0;
    for(; i < TOTAL_CREATED_JOBS; i++)
    {
        totalTimeUsingCPU += jobContainer[i].C;
        totalTimeSpentWaiting += jobContainer[i].currentWaitTime;
        totalTurnAroundTime += jobContainer[i].finishTime - jobContainer[i].A;
    }

    printf("summary data\n");
    printf("\tfinishing time: %i\n", CURRENT_TIME_CYCLE - 1);
    printf("\tCPU Utilization: %6f\n", totalTimeUsingCPU/(CURRENT_TIME_CYCLE - 1));
    printf("\tIO Utilization: %6f\n", TOTAL_NUMBER_OF_BLOCKED_CYCLES/(CURRENT_TIME_CYCLE - 1));
    printf("\tthroughput: %6f processes per hundred cycles\n", 100.00*TOTAL_CREATED_JOBS/(CURRENT_TIME_CYCLE - 1));
    printf("\taverage turnaround time: %6f\n", totalTurnAroundTime/TOTAL_CREATED_JOBS);
    printf("\taverage wait time: %6f\n", totalTimeSpentWaiting/TOTAL_CREATED_JOBS);

} //end of summary printing function

/* Wrapper functions */

void firstComefirstServe(struct Job jobContainer[], struct Job terminatedJobContainer[])
{
    printf("----------First Come First Serve----------\n");
    printContainer(jobContainer, 0);
    runFirstComeFirstServe(jobContainer, terminatedJobContainer, 0);
    printContainer(terminatedJobContainer, 1);
    printPerProcessAttributes(jobContainer);
    printSummary(jobContainer);
    reset(jobContainer, terminatedJobContainer);
    printf("----------First Come First Serve----------\n");
} //end of firstComeFirstServe wrapper

void roundRobin(struct Job jobContainer[], struct Job terminatedJobContainer[])
{
    printf("----------roundRobin----------\n");
    printContainer(jobContainer, 0);
    runFirstComeFirstServe(jobContainer, terminatedJobContainer, 1);
    printContainer(terminatedJobContainer, 1);
    printPerProcessAttributes(jobContainer);
    printSummary(jobContainer);
    reset(jobContainer, terminatedJobContainer);
    printf("----------roundRobin----------\n");
} //end of roundRobin wrapper


void uniProgrammed(struct Job jobContainer[], struct Job terminatedJobContainer[])
{
    printf("----------UniProgrammed----------\n");
    printContainer(jobContainer, 0);
    runFirstComeFirstServe(jobContainer, terminatedJobContainer, 2);
    printContainer(terminatedJobContainer, 1);
    printPerProcessAttributes(jobContainer);
    printSummary(jobContainer);
    reset(jobContainer, terminatedJobContainer);
    printf("----------UniProgrammed----------\n");
} //end of uniProgrammed wrapper


void shortestJobFirst(struct Job jobContainer[], struct Job terminatedJobContainer[])
{
    printf("----------Shortest Job First----------\n");
    printContainer(jobContainer, 0);
    runFirstComeFirstServe(jobContainer, terminatedJobContainer, 3);
    printContainer(terminatedJobContainer, 1);
    printPerProcessAttributes(jobContainer);
    printSummary(jobContainer);
    reset(jobContainer, terminatedJobContainer);
    printf("----------Shortest Job First----------\n");
} //end of shortestJobFirst wrapper

int main (int argc, char* argv[])
{
    int indicatorNumber;
    FILE* mixesInput = NULL;
    char* mixesFilePath;
    //check for flags
    if(argc == 3)
    {
        mixesFilePath = argv[2];
        if (strcmp(argv[1], "--verbose") == 0)
        {
            IS_VERBOSE = true;
            SHOW_RANDOM = false;
        }
        else
        {
            IS_VERBOSE = true;
            SHOW_RANDOM = true;
        }
    }
    else
        mixesFilePath = argv[1];
    //reading in mixes
    mixesInput = fopen(mixesFilePath,"r");
    fscanf(mixesInput, "%i", &indicatorNumber); //read in indicator number
    //creating all jobs
    int i;
    struct Job jobContainer[indicatorNumber];
    struct Job terminatedJobContainer[indicatorNumber];
    for (i = 0; i < indicatorNumber; ++i)
    {
        int currentJobA;
        int currentJobB;
        int currentJobC;
        int currentJobM;
        //isolates for the integers, reads in each integer as currentJobA,B,C,or M
        fscanf(mixesInput, " %*c%i %i %i %i%*c", &currentJobA, &currentJobB, &currentJobC, &currentJobM);

        jobContainer[i].A = currentJobA;
        jobContainer[i].B = currentJobB;
        jobContainer[i].C = currentJobC;
        jobContainer[i].M = currentJobM;
        jobContainer[i].currentCPUBurst = 0; //TODO MAY NEED TO INITIALIZE HERE
        jobContainer[i].currentCPUTime = 0;
        jobContainer[i].currentIOBlocked = 0;
        jobContainer[i].currentIOBurst = 0;
        jobContainer[i].currentStatus = 0;
        jobContainer[i].currentWaitTime = 0;
        jobContainer[i].finishTime = -1;
        jobContainer[i].jobID = i;

        jobContainer[i].quantum = 2;
        jobContainer[i].isFirstTimeRunning = true;

        jobContainer[i].nextReady = NULL;
        jobContainer[i].nextReadySuspended = NULL;

        ++TOTAL_CREATED_JOBS;
    }//created all jobs from input
    fclose(mixesInput);
    //begins job scheduler simulation

    firstComefirstServe(jobContainer, terminatedJobContainer);
    roundRobin(jobContainer, terminatedJobContainer);
    uniProgrammed(jobContainer, terminatedJobContainer);
    shortestJobFirst(jobContainer, terminatedJobContainer);

    return 0;
} //end of main function