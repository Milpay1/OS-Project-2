#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <sys/wait.h>
#include <signal.h>

// Semaphore declarations
sem_t mutex;

// Function prototypes
void ParentProcess(int *BankAccount);
void ChildProcess(int *BankAccount);

int main() {
    int ShmID;
    int *ShmPTR;
    pid_t pid;

    // Set up shared memory
    ShmID = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666);
    if (ShmID < 0) {
        perror("shmget error");
        exit(1);
    }

    ShmPTR = (int *)shmat(ShmID, NULL, 0);
    if ((long)ShmPTR == -1) {
        perror("shmat error");
        exit(1);
    }

    *ShmPTR = 100; // Initialize BankAccount to $100

    // Initialize semaphore
    sem_init(&mutex, 1, 1);

    // Fork to create parent and child processes
    pid = fork();
    if (pid < 0) {
        perror("fork error");
        exit(1);
    } else if (pid == 0) {
        ChildProcess(ShmPTR); // Child process
    } else {
        ParentProcess(ShmPTR); // Parent process
    }

    // Detach and clean up shared memory (this won't run since the program is indefinite)
    shmdt(ShmPTR);
    shmctl(ShmID, IPC_RMID, NULL);
    sem_destroy(&mutex);

    return 0;
}

void ParentProcess(int *BankAccount) {
    int localBalance, amount;

    while (1) { // Infinite loop
        sleep(rand() % 6); // Sleep for 0-5 seconds
        printf("Dear Old Dad: Attempting to Check Balance\n");

        sem_wait(&mutex); // Lock the critical section

        localBalance = *BankAccount;

        if (rand() % 2 == 0) { // 50% chance to attempt deposit
            if (localBalance < 100) {
                amount = rand() % 101; // Random deposit amount: 0-100
                if (amount % 2 == 0) { // Only deposit if amount is even
                    localBalance += amount;
                    printf("Dear Old Dad: Deposits $%d / Balance = $%d\n", amount, localBalance);
                } else {
                    printf("Dear Old Dad: Doesn't have any money to give\n");
                }
                *BankAccount = localBalance; // Update shared memory
            } else {
                printf("Dear Old Dad: Thinks Student has enough Cash ($%d)\n", localBalance);
            }
        } else {
            printf("Dear Old Dad: Last Checking Balance = $%d\n", localBalance);
        }

        sem_post(&mutex); // Unlock the critical section
    }
}

void ChildProcess(int *BankAccount) {
    int localBalance, need;

    while (1) { // Infinite loop
        sleep(rand() % 6); // Sleep for 0-5 seconds
        printf("Poor Student: Attempting to Check Balance\n");

        sem_wait(&mutex); // Lock the critical section

        localBalance = *BankAccount;

        if (rand() % 2 == 0) { // 50% chance to attempt withdrawal
            need = rand() % 51; // Random withdrawal amount: 0-50
            printf("Poor Student needs $%d\n", need);

            if (need <= localBalance) {
                localBalance -= need;
                printf("Poor Student: Withdraws $%d / Balance = $%d\n", need, localBalance);
            } else {
                printf("Poor Student: Not Enough Cash ($%d)\n", localBalance);
            }
            *BankAccount = localBalance; // Update shared memory
        } else {
            printf("Poor Student: Last Checking Balance = $%d\n", localBalance);
        }

        sem_post(&mutex); // Unlock the critical section
    }
}
