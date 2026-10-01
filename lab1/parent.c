#include <stdint.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>

static char CHILD_PROGRAM_NAME[] = "children";

int main(int argc, char** argv) {
	(void)argc;
	(void)argv;

	char progpath[1024];
	ssize_t len = readlink("/proc/self/exe", progpath, sizeof(progpath) - 1);
	if (len == -1) {
		const char msg[] = "error: failed to read full program path\n";
		write(STDERR_FILENO, msg, sizeof(msg));
		exit(EXIT_FAILURE);
	}
	while (progpath[len] != '/') len--;
	progpath[len] = '\0';

	int pipe1[2], pipe2[2];
	if (pipe(pipe1) == -1 || pipe(pipe2) == -1) {
		const char msg[] = "error: failed to create pipe\n";
		write(STDERR_FILENO, msg, sizeof(msg) - 1);
		exit(EXIT_FAILURE);
	}

	char path[1024];

	size_t i;
	for (i = 0; i < strlen(progpath) && i + 1 < sizeof(path); i++)
		path[i] = progpath[i];
	if (i + 1 < sizeof(path)) path[i++] = '/';
	for (size_t j = 0; j < strlen(CHILD_PROGRAM_NAME) && i + 1 < sizeof(path); j++)
		path[i++] = CHILD_PROGRAM_NAME[j];
	path[i] = '\0';

	char file1[256];
	char file2[256];

	ssize_t x = read(STDIN_FILENO, file1, sizeof(file1) - 1);
	if (x <= 0) {
		const char msg[] = "error: failed to read file1 name\n";
		write(STDERR_FILENO, msg, sizeof(msg) - 1);
		exit(EXIT_FAILURE);
	}
	file1[x] = '\0';
	if (x > 0 && file1[x - 1] == '\n') file1[x - 1] = '\0';

	x = read(STDIN_FILENO, file2, sizeof(file2) - 1);
	if (x <= 0) {
		const char msg[] = "error: failed to read file2 name\n";
		write(STDERR_FILENO, msg, sizeof(msg) - 1);
		exit(EXIT_FAILURE);
	}
	file2[x] = '\0';
	if (x > 0 && file2[x - 1] == '\n') file2[x - 1] = '\0';

	int fd1 = open(file1, O_WRONLY | O_CREAT | O_TRUNC, 0600);
	if (fd1 == -1) {
		const char msg[] = "error: failed to open file1\n";
		write(STDERR_FILENO, msg, sizeof(msg) - 1);
		exit(EXIT_FAILURE);
	}

	int fd2 = open(file2, O_WRONLY | O_CREAT | O_TRUNC, 0600);
	if (fd2 == -1) {
		const char msg[] = "error: failed to open file2\n";
		write(STDERR_FILENO, msg, sizeof(msg) - 1);
		exit(EXIT_FAILURE);
	}

	const pid_t child1 = fork();

	switch (child1) {
	case -1: {
		const char msg[] = "error: failed to spawn new process [child1]\n";
		write(STDERR_FILENO, msg, sizeof(msg) - 1);
		exit(EXIT_FAILURE);
	} break;

	case 0: {
		
		close(pipe1[1]);
		close(pipe2[0]);
		close(pipe2[1]);
		close(fd2);

		dup2(pipe1[0], STDIN_FILENO);
		close(pipe1[0]);

		dup2(fd1, STDOUT_FILENO);
		close(fd1);

		char* const args[] = { CHILD_PROGRAM_NAME, NULL };

		int32_t status = execv(path, args);
		if (status == -1) {
			const char msg[] = "error: failed to exec into new executable image [child1]\n";
			write(STDERR_FILENO, msg, sizeof(msg) - 1);
			exit(EXIT_FAILURE);
		}
	} break;
	default:
		break;
	}

	const pid_t child2 = fork();

	switch (child2) {
	case -1: {
		const char msg[] = "error: failed to spawn new process [child2]\n";
		write(STDERR_FILENO, msg, sizeof(msg) - 1);
		exit(EXIT_FAILURE);
	} break;

	case 0: {
		
		close(pipe1[0]);
		close(pipe1[1]);
		close(pipe2[1]);
		close(fd1);

		dup2(pipe2[0], STDIN_FILENO);
		close(pipe2[0]);

		dup2(fd2, STDOUT_FILENO);
		close(fd2);

		char* const args[] = { CHILD_PROGRAM_NAME, NULL };

		int32_t status = execv(path, args);
		if (status == -1) {
			const char msg[] = "error: failed to exec into new executable image [child2]\n";
			write(STDERR_FILENO, msg, sizeof(msg) - 1);
			exit(EXIT_FAILURE);
		}
	} break;
	default:
		break;
	}
	
	char buf[4096];
	ssize_t bytes;

	close(pipe1[0]);
	close(pipe2[0]);
	close(fd1);
	close(fd2);

	while ((bytes = read(STDIN_FILENO, buf, sizeof(buf))) > 0) {
		if (buf[0] == '\n') break;

		size_t linelen = (size_t)bytes;
		if (linelen > 0 && buf[linelen - 1] == '\n') linelen--;

		int t = (linelen > 10) ? pipe2[1] : pipe1[1];
		write(t, buf, bytes);
	}

	close(pipe1[1]);
	close(pipe2[1]);

	wait(NULL);
	wait(NULL);

	return 0;
}
