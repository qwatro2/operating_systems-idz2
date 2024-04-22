#include "../common/arguments.h"
#include "../common/help.h"

#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <time.h>
#include <sys/wait.h>

typedef struct {
    int areas[1024];
    int treasure;
    bool founded;
} island;

arguments args;
pid_t parent;
pid_t children[1024];
island *shm;
int shmid;
int semid;

void cleanup() {
    while(wait(NULL) > 0);
    shmdt(shm);
    shmctl(shmid, IPC_RMID, NULL);
    semctl(semid, 0, IPC_RMID);
    exit(0);
}

void sigint_handler() {
    if (getpid() != parent) {
    	exit(0);
    }
    cleanup();
}

void stop() {
    if (getpid() == parent) {
        for (int i = 1; i <= args.number_of_groups; ++i) {
    	    printf("Group #%d returned to shore.\n", i);
            kill(children[i-1], SIGKILL);
        }
    }
    cleanup();
}

int main(int argc, const char *argv[]) {
    if (parse_help(argc, argv)) {
        print_help(argv[0]);
        return 0;
    }

    if (!try_parse_arguments(argc, argv, &args)) {
        printf("\033[31mERROR\033[0m: wrong arguments\n");
        print_help(argv[0]);
        return 0;
    }

    if (!is_argc_correct(&args)) {
        printf("\033[31mERROR\033[0m: both arguments must be greater than 0 and number_of_areas must be greater than number_of_groups\n");
        return 0;
    }

    signal(SIGINT, sigint_handler);
    signal(SIGUSR1, stop);

    key_t key = ftok("sem_file", 1);
    semid = semget(key, 1, 0644 | IPC_CREAT);
    semctl(semid, 0, SETVAL, 0);

    key = ftok("shm_file", 1);
    shmid = shmget(key, sizeof(island), 0644 | IPC_CREAT);
    shm = (island*) shmat(shmid, NULL, 0);
    
    for (int i = 0; i < args.number_of_areas; ++i) {
        shm->areas[i] = -1;
    }
    
    srand(time(NULL));
    shm->treasure = rand() % args.number_of_areas;
    printf("Treasure is burried in area: %d\n", shm->treasure);

    pid_t pid;
    parent = getpid();
    struct sembuf buf;
    
    for (int i = 1; i <= args.number_of_groups; ++i) {
        pid = fork();
        if (pid < 0) {
            printf("Fork error!\n");
            exit(-1);
        }

        if (pid > 0) {
            children[i - 1] = pid;
            continue;
        }
        
        for (int j = 0; j < args.number_of_areas; ++j) {
            buf.sem_num = 0;
            buf.sem_op = -1;
            buf.sem_flg = 0;
            semop(semid, &buf, 1);
            if (shm->areas[j] == -1) {
                if (shm->treasure == j) {
                    shm->areas[j] = 1;
                    shm->founded = true;
                    printf("Group #%d: found treasure in area %d! Everyone, return to shore!\n", i, j + 1);
                    kill(parent, SIGUSR1);
                    buf.sem_num = 0;
                    buf.sem_op = -1;
                    buf.sem_flg = 0;
                    semop(semid, &buf, 1);
                } else {
                    shm->areas[j] = 0;
                    if (!shm->founded) {
                        printf("Group #%d: there is no chest in area %d! Keep searching!\n", i, j + 1);
                    }
                }
                sleep(1 + rand() % 3);
            }
            buf.sem_num = 0;
            buf.sem_op = 1;
            buf.sem_flg = 0;
            semop(semid, &buf, 1);
        }
    }

    for (int i = 0; i < args.number_of_groups; ++i) {
        buf.sem_num = 0;
        buf.sem_op = 1;
        buf.sem_flg = 0;
        semop(semid, &buf, 1);
    }

    // Wait for children
    while(wait(NULL) > 0);
    
    return 0;
}
