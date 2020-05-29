#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <signal.h>
#include<time.h>


#define BarbersForWomen 2    //barbers serving for female N1
#define BarbersForMen 3      //barbers serving for male   N2
#define BarbersForBoth 4     //barbers serving for both female and male N3
#define ChairNumber 20       //the number of chairs in waitting room

#define ClientNumber 12

const key_t MutexKey =  0x1000;                             // unique key for mutex semaphore
const key_t BarberSemaphoreKey = 0x6060;                    // unique key for semaphores for barbers
const key_t CommonResourceKey = 0x2060;                     // unique key for shared memory semaphore

struct commonResource
{
    int maleClients;                        //the number of male client in queue
    int femaleClients;                      //the number of female client in queue
    int maleBarber[BarbersForMen];          //array for male barbers process
    int femaleBarber[BarbersForWomen];      //array for female barbers process
    int bothBarber[BarbersForBoth];         //array for both male and female process
}*C_R;


void barberWorkDistrbution(int i, int barbers);
void barber(int barberType, int allBarbers, int barberID);
void client(int allBarbers, int barberIndex);
void terminateProcess(int CreatedProcess, pid_t *processList);
void lock(int semid, int idx);
void unlock(int semid, int idx);

int main()
{
    
    int allBarbers = BarbersForBoth + BarbersForWomen + BarbersForMen;  // all barbers
    int semaphoreID;                                                    // semphore for common resource
    pid_t processList[allBarbers+ClientNumber];
    
    semaphoreID = shmget(CommonResourceKey, sizeof(struct commonResource), IPC_CREAT|0666);
    if(semaphoreID < 0)
    {
        perror("\nFailure of creation of semaphore.\n");
        exit(1);
    }
    
    //access memory
    C_R = shmat(semaphoreID, NULL, 0);
    if(C_R == (void*) - 1)              // tramsform -1 to 0XFFFFFFFF
    {
        perror("\nFailure to access.\n");
        exit(1);
    }
    
    //initial the data
    C_R->maleClients = (rand()%(ClientNumber-1) + 1);
    C_R->femaleClients = ClientNumber - C_R->maleClients;
    for(int i = 0; i < BarbersForMen; i++)
    {
        C_R->maleBarber[i] = -1;
    }
    for(int i = 0; i < BarbersForWomen; i++)
    {
        C_R->femaleBarber[i] = -1;
    }
    for(int i = 0; i < BarbersForBoth; i++)
    {
        C_R->bothBarber[i] = -1;
    }
    
    printf("There are %d - male in the queue.\n", C_R->maleClients);
    printf("There are %d - female in the queue.\n", C_R->femaleClients);
    
    
    //create mutex
    int mutex = semget(MutexKey, 1, IPC_CREAT|0666);
    if(mutex < 0)
    {
        perror("\nMutex not created!!!\n");
        exit(1);
    }
    
    union semun
    {
        int value;
        unsigned int *array;
    }semaphoreUN;
    
    semaphoreUN.value = 1;
    if(semctl(mutex, 0, SETVAL, semaphoreUN) < 0)    // set semaphore value, set value in fourth parameter
    {
        perror("\nsemctl: mutex value failled to set!!!\n");
        exit(1);
    }
    
    int barberSem = semget(BarberSemaphoreKey, allBarbers, IPC_CREAT|0666);
    if(barberSem < 0)
    {
        perror("\nbarbers semaphore are not created!!!\n");
        exit(1);
    }
    
    unsigned int zeros[allBarbers];
    for(int i = 0; i < allBarbers; i++)
    {
        zeros[i] = 0;
    }
    
    semaphoreUN.array = zeros;
    if(semctl(barberSem, 0, SETALL, semaphoreUN) < 0)  //set all of semaphore value
    {
        perror("\nsemctl: barber semphores values failed to set!!!\n");
        exit(1);
    }
    
    for(int i = 0; i < (allBarbers+ClientNumber); i++)
    {
        pid_t pid = fork();
        if(pid < 0)
        {
            perror("\nFailure to create child process.\n");
            exit(1);
        }
        else if(pid > 0)            //parent process
        {
            processList[i] = pid;
        }
        else
        {
            barberWorkDistrbution(i, allBarbers);
            return 0;
        }
    }
    
    sleep(20);
    terminateProcess((allBarbers+ClientNumber-1), processList);
    return 0;
}


void barberWorkDistrbution(int i, int barbers)    // i is index and barbers is the number of all of barbers
{
    if(i%2 == 1 && i < (BarbersForWomen+BarbersForMen))       //odd index are male barbers
    {
        printf("Created male Barber [%d]\n", getpid());
        barber(1, barbers, i);
    }
    else if(i%2 == 0 && i < (BarbersForWomen+BarbersForMen))  // even index are female barbers
    {
        printf("Created female Barber [%d]\n", getpid());
        barber(2, barbers, i);
    }
    else if(i >= (BarbersForWomen+BarbersForMen) && i < barbers) // rest barbers are serving male or female
    {
        printf("Created bothBarber [%d]\n", getpid());
        barber(3, barbers, i);
    }
    else
    {
        printf("Created process for client generation [%d]\n", getpid());
        client(barbers, i);
    }
}
void barber(int barberType, int allBarbers, int barberID)
{
    int cuttingTime = 0;
    while(TRUE)
    {
        int mutex = semget(MutexKey, 1, 0666);
        int barberSem = semget(BarberSemaphoreKey, allBarbers, 0666);
        if(mutex < 0 || barberSem < 0)
        {
            perror("Error");
            exit(1);
        }
        lock(mutex, 0);
        
        switch (barberType) {
            case 1:
                if(C_R->maleClients > 0)
                {
                    C_R->maleClients--;
                    cuttingTime = 2;
                    printf("male barber [%d] is cutting hair\n",getpid());
                }
                else
                {
                    for(int i = 0; i < BarbersForMen; i++)
                    {
                        if(C_R->maleBarber[i] == -1)
                        {
                            C_R->maleBarber[i] = barberID;
                        }
                    }
                    printf("male barber [%d] sleep\n", barberID);
                    unlock(mutex, 0);
                    lock(barberSem, barberID);
                }
                break;
                case 2:
                    if(C_R->femaleClients > 0)
                    {
                        C_R->femaleClients--;
                        cuttingTime = 2;
                        printf("female barber [%d] is cutting hair\n",getpid());
                    }
                    else
                    {
                        for(int i = 0; i < BarbersForWomen; i++)
                        {
                            if(C_R->femaleBarber[i] == -1)
                            {
                                C_R->femaleBarber[i] = barberID;
                            }
                        }
                        printf("female barber [%d] sleep\n", barberID);
                        unlock(mutex, 0);
                        lock(barberSem, barberID);
                    }
                    break;
                case 3:
                    if(C_R->femaleClients == 0 && C_R->maleClients == 0)
                    {
                        for(int i = 0; i < BarbersForBoth; i++)
                        {
                            if(C_R->bothBarber[i] == -1)
                            {
                                C_R->bothBarber[i] = barberID;
                            }
                        }
                        printf("bothBarber [%d] sleep\n", barberID);
                        unlock(mutex, 0);
                        lock(barberSem, barberID);
                    }
                    else if(C_R->maleClients > C_R->femaleClients)
                    {
                        C_R->maleClients--;
                        cuttingTime = 2;
                        printf("bothBarber [%d] is cutting male client hair\n", getpid());
                    }
                    else if(C_R->maleClients <= C_R->femaleClients)
                    {
                        C_R->femaleClients--;
                        cuttingTime = 2;
                        printf("bothBarber [%d] is cutting female client hair\n",getpid());
                    }
                    break;
        }
        printf("Client in queue: male - %d, female - %d\n",C_R->maleClients, C_R->femaleClients);
        unlock(mutex, 0);
        sleep(cuttingTime);
    }
}

void client(int allBarbers, int barberIndex)
{
    int waitTime = 0;
    while(TRUE)
    {
        int mutex = semget(MutexKey, 1, 0666);
        int barberSem = semget(BarberSemaphoreKey, allBarbers, 0666);
        if(barberSem < 0 || mutex < 0)
        {
            perror("Error");
            exit(0);
        }
        lock(mutex, 0);
        int newClientNum = (rand()%2) + 1;          // if newClientNum = 1 means male client
                                                    // if newClientNum = 2 means female client
        if((C_R->maleClients + C_R->femaleClients) >= ChairNumber)
        {
            // new client leave
            printf("newClient left cause no space in waitting room!!!\n");
        }
        else if(newClientNum == 1)
        {
            printf("New male client go to waitting room!!!\n");
            if(C_R->maleClients >= 0 && C_R->maleBarber[0] > -1 && BarbersForMen >= 1)
            {
                printf("Wake up [%d] maleBarber", C_R->maleBarber[0]);
                unlock(barberSem, C_R->maleBarber[0]);
                for(int i = 0; i < BarbersForMen + 1; i++)
                {
                    C_R->maleBarber[i] = C_R->maleBarber[i+1];
                }
            }
            else if(C_R->maleClients >= 0 && C_R->bothBarber[0] > -1 && BarbersForBoth >= 1)
            {
                printf("Wake up [%d] bothBarber!!!\n", C_R->bothBarber[0]);
                unlock(barberSem, C_R->bothBarber[0]);
                for(int i = 0; i < BarbersForBoth + 1; i++)
                {
                    C_R->bothBarber[i] = C_R->bothBarber[i+1];
                }
            }
            C_R->maleClients++;
        }
        else if(newClientNum == 2)
        {
            printf("New female client go to watting room!!!\n");
            if(C_R->femaleClients >= 0 && C_R->femaleBarber[0] > -1 && BarbersForWomen >= 1)
            {
                printf("Wake up [%d] femaleBarber!!!\n", C_R->femaleBarber[0]);
                unlock(barberSem, C_R->femaleBarber[0]);
                for(int i = 0; i < BarbersForWomen + 1; i++)
                {
                    C_R->femaleBarber[i] = C_R->femaleBarber[i+1];
                }
            }
            else if(C_R->femaleClients >= 0 && C_R->bothBarber[0] > -1 && BarbersForBoth >= 1)
            {
                printf("Wake up [%d] bothBarber!!!\n", C_R->bothBarber[0]);
                unlock(barberSem, C_R->bothBarber[0]);
                for(int i = 0; i < BarbersForBoth + 1; i++)
                {
                    C_R->bothBarber[i] = C_R->bothBarber[i+1];
                }
            }
            C_R->femaleClients++;
        }
        printf("Client in queue: male - %d, female - %d\n", C_R->maleClients, C_R->femaleClients);
        unlock(mutex, 0);
        waitTime = 3;
        sleep(waitTime);
    }
}

//function for lock given semaphore
void lock(int semid, int idx)
{
    struct sembuf p = {idx, -1, SEM_UNDO};
    
    if(semop(semid, &p, 1) < 0)
    {
        perror("semop lock");
        exit(1);
    }
}

//function for unlock given semaphore
void unlock(int semid, int idx)
{
    struct sembuf v = {idx, +1, SEM_UNDO};

    if(semop(semid, &v, 1) < 0)
    {
        perror("semop unlock");
        exit(1);
    }
}

void terminateProcess(int CreatedProcess, pid_t *processList)
{
    for(int i = 0; i < CreatedProcess; i++)
    {
        kill(processList[i],SIGTERM);
    }
}
