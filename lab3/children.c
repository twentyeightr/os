#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#define SHM_SIZE 8192

typedef struct {
    bool from_parent;
    bool done;
    int target;
    uint32_t length;
    char data[SHM_SIZE - 32];
} SharedData;

int main(int argc, char** argv) {
    if (argc < 5) {
        fprintf(stderr, "usage: %s <1|2> <shm_name> <sem_empty> <sem_full>\n", argv[0]);
        return 1;
    }

    int my_id = atoi(argv[1]);
    const char* shm_name = argv[2];
    const char* sem_empty_name = argv[3];
    const char* sem_full_name = argv[4];

    int shm_fd = shm_open(shm_name, O_RDWR, 0600);
    if (shm_fd == -1) {
        const char msg[] = "error: failed to open SHM\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        _exit(EXIT_FAILURE);
    }

    SharedData* shm = mmap(NULL, sizeof(SharedData), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm == MAP_FAILED) {
        const char msg[] = "error: failed to map SHM\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        _exit(EXIT_FAILURE);
    }

    sem_t* sem_empty = sem_open(sem_empty_name, 0);
    sem_t* sem_full = sem_open(sem_full_name, 0);
    if (sem_empty == SEM_FAILED || sem_full == SEM_FAILED) {
        const char msg[] = "error: failed to open semaphore\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        _exit(EXIT_FAILURE);
    }

    while (true) {
        sem_wait(sem_full);

        if (shm->done) {
            sem_post(sem_empty);
            break;
        }

        if (shm->target == my_id) {
            uint32_t len = shm->length;
            char local_buf[SHM_SIZE];
            memcpy(local_buf, shm->data, len);
            local_buf[len] = '\0';

            sem_post(sem_empty);

            for (uint32_t i = 0; i < len / 2; i++) {
                char t = local_buf[i];
                local_buf[i] = local_buf[len - 1 - i];
                local_buf[len - 1 - i] = t;
            }

            printf("[Child %d] %s\n", my_id, local_buf);
            fflush(stdout);
        }
        else {
            sem_post(sem_full);
        }
    }

    munmap(shm, sizeof(SharedData));
    close(shm_fd);
    sem_close(sem_empty);
    sem_close(sem_full);
    return 0;
}
