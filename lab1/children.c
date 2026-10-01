#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char** argv) {
	char buf[4096];
	ssize_t bytes;

	(void)argc;
	(void)argv;

	while ((bytes = read(STDIN_FILENO, buf, sizeof(buf))) > 0) {
		ssize_t len = bytes;
		if (len > 0 && buf[len - 1] == '\n') len--;

		for (ssize_t i = 0; i < len / 2; i++) {
			char t = buf[i];
			buf[i] = buf[len - 1 - i];
			buf[len - 1 - i] = t;
		}

		write(STDOUT_FILENO, buf, len);
		write(STDOUT_FILENO, "\n", 1);
	}

	if (bytes < 0) {
		const char msg[] = "error: failed to read from stdin\n";
		write(STDERR_FILENO, msg, sizeof(msg) - 1);
		exit(EXIT_FAILURE);
	}

	return 0;
}
