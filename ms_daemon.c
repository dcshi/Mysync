#include "ms_daemon.h"
#include "ms_core.h"

int ms_pid;

int ms_daemon()
{
	int  fd;

	switch (fork()) {
		case -1:
			ms_error("fork() failed");
			return -1;
		case 0:
			break;
		default:
			exit(0);
	}

	ms_pid = getpid();

	if (setsid() == -1) {
		ms_error("setsid() failed");
		return -1;
	}

	umask(0);

	fd = open("/dev/null", O_RDWR);
	if (fd == -1) {
		ms_error("open(\"/dev/null\") failed");
		return -1;
	}

	if (dup2(fd, STDIN_FILENO) == -1) {
		ms_error("dup2(STDIN) failed");
		return -1;
	}

	if (dup2(fd, STDOUT_FILENO) == -1) {
		ms_error("dup2(STDOUT) failed");
		return -1;
	}

	if (dup2(fd, STDERR_FILENO) == -1) {
		ms_error("dup2(STDERR) failed");
		return -1;
	}

	if (fd > STDERR_FILENO) {
		if (close(fd) == -1) {
			ms_error("close() failed");
			return -1;
		}
	}

	return 0;
}

