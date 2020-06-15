#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/types.h>

#define N               5
#define LEFT            ( i + N - 1 ) % N
#define RIGHT           ( i + 1 ) % N
#define THINKING        1                 //flag set thinking as state number 1
#define HUNGRY          2                 //flag set hungry as state number 2
#define EATING          3                 //flag set eating as state number 3

#define FOOD_LIMIT      3
#define EATING_TIME     1
#define THINKING_TIME   2

 /*
  The pthread_mutex_lock () function locks the mutex object specified by mutex. If the mutex has been locked, the thread calling this function is blocked until the mutex is available. The mutex object specified by the parameter mutex becomes the locked state when the function returns, and the calling thread of the function becomes the owner of the mutex object.
  */
#define up pthread_mutex_unlock
/*
We can release the mutex reference mutex object. The manner in which the mutex is released depends on the type attribute of the mutex. If more than one thread is blocked by the mutex object when pthread_mutex_unlock () is called, the dispatching strategy determines the thread that gets the mutex when the mutex becomes available. For a mutex of type PTHREAD_MUTEX_RECURSIVE, the mutex becomes available when the count reaches zero and the calling thread no longer locks on the mutex.
*/
#define down pthread_mutex_lock


pthread_mutex_t m;                  //main mutex initialized to 1 for critical regoin
int state[N];                       //initialized to THINKING's
pthread_mutex_t s[N];               //initialized to 0's initialized philosopher fork mutex
pthread_t philosophers[N];
char *philosopher_names[5] ={
    "Philosopher[1]","Philosopher[2]","Philosopher[3]","Philosopher[4]","Philosopher[5]"
};
int h[N];                           //counter for each philosopher eat times

void grab_forks(int i);
void put_away_forks(int i);
void put_away_forks(int i);
void *table_lifecycle(void *i);


int main() {
    
    int i = 0;
    //allocate pointers for numbers of each philosopher
    int *num = (int *)malloc(5*sizeof(int));
    //pthread_mutex_init(pthread_mutex_t * mutex, pthread_mutexattr_t *attr);
    //initialized main mutex variable is m, attr is attribute, NULL means default attribute
    pthread_mutex_init(&m, NULL);
    
    for(i = 0; i < N; i++)
    {
        //initialized state to thinking
        state[i] = THINKING;
        //initialized mutex s[N], initialized fork
        pthread_mutex_init(&s[i], NULL);
        //initialized s[i] to 0
        down(&s[i]);
        num[i] = i;
        /*
         parameters of pthread_create: first is the pointer points to the threads identifier, second set the attributes of the threads, third parameter for the start address of the threads, last one is the parameter of the executing function.
         */
        //create the threads return 0 if success create
        if(pthread_create(&philosophers[i], NULL, table_lifecycle, (num+i)) != 0)
        {
            fprintf(stderr, "\nERROR: pthread_create() failed. Cannot create philosopher!\n");
            return 1;
        }
    }
    //release free the memory
    free(num);
    for(i = 0; i < N; i++)
    {
        //wait until the thread finish.
        if(pthread_join(philosophers[i], NULL) != 0)
        {
            fprintf(stderr, "\nERROR: pthread_join() falied.");
            exit(1);
        }
    }
    //destroy the mutex(lock), release after finish
    pthread_mutex_destroy(&s[i]);
    
    for(i = 0; i < N; i++)
    {
        int mm = (int) i;
        printf("%s thinked %d times..in total!!!\n", philosopher_names[mm], h[i]);
        //destroy the mutex(lock), release after finish
        pthread_mutex_destroy(&s[i]);
    }
    pthread_exit(NULL);
    return 0;
}


void test(int i)
{
    if(state[i] == HUNGRY && state[LEFT] != EATING && state[RIGHT] != EATING)
    {
        //change state to eat
        state[i] = EATING;
        //unlock fork for person
        up(&s[i]);
    }
}

void grab_forks(int i)
{
    //lock main mutex, "one philosopher going to grab and eat" which in the eating action now, so
    //for other threads, they have been locked and have to wait
    down(&m);
    //change the state to HUNGRY
    state[i] = HUNGRY;
    //counting for each philosopher eating times
    h[i]++;
    test(i);
    //unlocking the main mutex(the grab action is going to finish) so order food now
    up(&m);
    //lock the fork that block others to use
    down(&s[i]);
    //taking left and right fork
}
void put_away_forks(int i)
{
    //put away left and right forks
    //locking the main mutex so no ordering food
    down(&m);
    state[i] = THINKING;
    //check neighbour left
    test(LEFT);
    //check neighbour right
    test(RIGHT);
    //unlock the main mutex so that the other one can access the "action of eating" ......reverse order of grab
    up(&m);                          //wake up sleeping thread
}

void *table_lifecycle(void *i)      //philosopher action
{
    int meals_left = FOOD_LIMIT;
    //number of the philosopher
    int mm = *(int*) i;
    printf("%s came to the table. ++\n", philosopher_names[mm]);
    //check if meal has finish or not
    while(meals_left != 0)
    {
        printf("%s is thinking........\n", philosopher_names[mm]);
        //sleep time for thinking
        sleep(THINKING_TIME);
        grab_forks(mm);
        printf("%s is eating the #. [ %d ] meal.\n", philosopher_names[mm], (FOOD_LIMIT-(--meals_left)));
        //sleep time for eating
        sleep(EATING_TIME);
        printf("%s have done meal [%d] \n", philosopher_names[mm], (FOOD_LIMIT-meals_left));
        put_away_forks(mm);
    }
}
