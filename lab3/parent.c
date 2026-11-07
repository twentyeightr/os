#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/wait.h>
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

int main() {
    char shm_name[64], sem_empty_name[64], sem_full_name[64];

    snprintf(shm_name, sizeof(shm_name), "/shm_%d", getpid());
    snprintf(sem_empty_name, sizeof(sem_empty_name), "/sem_empty_%d", getpid());
    snprintf(sem_full_name, sizeof(sem_full_name), "/sem_full_%d", getpid());

    int shm_fd = shm_open(shm_name, O_CREAT | O_RDWR, 0600);
    if (shm_fd == -1) {
        const char msg[] = "error: failed to create SHM\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        _exit(EXIT_FAILURE);
    }

    if (ftruncate(shm_fd, sizeof(SharedData)) == -1) {
        const char msg[] = "error: failed to resize SHM\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        _exit(EXIT_FAILURE);
    }

    SharedData* shm = mmap(NULL, sizeof(SharedData), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm == MAP_FAILED) {
        const char msg[] = "error: failed to map SHM\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        _exit(EXIT_FAILURE);
    }

    sem_t* sem_empty = sem_open(sem_empty_name, O_CREAT, 0600, 1);
    sem_t* sem_full = sem_open(sem_full_name, O_CREAT, 0600, 0);

    if (sem_empty == SEM_FAILED || sem_full == SEM_FAILED) {
        const char msg[] = "error: failed to create semaphore\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        _exit(EXIT_FAILURE);
    }

    shm->done = false;

    pid_t pid1 = fork();
    if (pid1 == 0) {
        execl("./children", "children", "1", shm_name, sem_empty_name, sem_full_name, NULL);
        const char msg[] = "error: failed to exec\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        _exit(EXIT_FAILURE);
    }
    else if (pid1 == -1) {
        const char msg[] = "error: failed to fork\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        _exit(EXIT_FAILURE);
    }

    pid_t pid2 = fork();
    if (pid2 == 0) {
        execl("./children", "children", "2", shm_name, sem_empty_name, sem_full_name, NULL);
        const char msg[] = "error: failed to exec\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        _exit(EXIT_FAILURE);
    }
    else if (pid2 == -1) {
        const char msg[] = "error: failed to fork\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        _exit(EXIT_FAILURE);
    }

    char buf[SHM_SIZE];
    while (true) {
        if (!fgets(buf, sizeof(buf), stdin)) break;
        if (buf[0] == '\n' || buf[0] == '\0') break;

        size_t len = strlen(buf);
        if (len > 0 && buf[len - 1] == '\n') buf[len - 1] = '\0';
        len = strlen(buf);
        if (len == 0) break;

        int target = (len > 10) ? 2 : 1;

        sem_wait(sem_empty);

        shm->from_parent = true;
        shm->target = target;
        shm->done = false;
        shm->length = len;
        memcpy(shm->data, buf, len);

        sem_post(sem_full);
    }

    sem_wait(sem_empty);
    shm->from_parent = true;
    shm->done = true;
    shm->target = 0;
    sem_post(sem_full);
    sem_post(sem_full);

    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);

    munmap(shm, sizeof(SharedData));
    close(shm_fd);
    shm_unlink(shm_name);
    sem_close(sem_empty);
    sem_unlink(sem_empty_name);
    sem_close(sem_full);
    sem_unlink(sem_full_name);

    return 0;
}
