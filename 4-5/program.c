#include "../common/arguments.h"
#include "../common/constants.h"
#include "../common/help.h"

#include <fcntl.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>
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
sem_t *sem;

void cleanup() {
    while(wait(NULL) > 0);
    munmap(shm, sizeof(int) * args.number_of_groups);
    shm_unlink(SHM_NAME);
    sem_close(sem);
    sem_unlink(SEM_NAME);
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
    	    printf("Group #%d: returned to shore.\n", i);
            kill(children[i - 1], SIGKILL);
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
    
    sem = sem_open(SEM_NAME, O_CREAT, 0644, 0);
    int shmfd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0644);
    ftruncate(shmfd, sizeof(island));
    shm = mmap(NULL, sizeof(island), PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);
    close(shmfd);
    
    for (int i = 0; i < args.number_of_areas; ++i) {
        shm->areas[i] = -1;
    }
    
    srand(time(NULL));
    shm->treasure = rand() % args.number_of_areas;
    printf("Treasure is burried in area: %d\n", shm->treasure + 1);

    pid_t pid;
    parent = getpid();
    
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
            sem_wait(sem);
            if (shm->areas[j] == -1) {
                if (shm->treasure == j) {
                    shm->areas[j] = 1;
                    shm->founded = true;
                    printf("Group #%d: found treasure in area %d! Everyone, return to shore!\n", i, j + 1);
                    kill(parent, SIGUSR1);
                    sem_wait(sem);
                } else {
                    shm->areas[j] = 0;
                    if (!shm->founded) {
                        printf("Group #%d: there is no chest in area %d! Keep searching!\n", i, j + 1);
                    }
                }
                sleep(1 + rand() % 3);
            }
            sem_post(sem);
        }
    }

    for (int i = 0; i < args.number_of_groups; ++i) {
        sem_post(sem);
    }

    while(wait(NULL) > 0);

    return 0;
}
