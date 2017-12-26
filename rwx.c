#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <libgen.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <X11/Xlib.h>

#define die(msg) do { perror(msg); \
	exit(EXIT_FAILURE); } while (0)

static void
chld(int num, siginfo_t *info, void *ctx)
{
	(void)num;
	(void)ctx;
	int wstatus;

	if (waitpid(info->si_pid, &wstatus, WNOHANG) > 0)
		exit(WEXITSTATUS(wstatus));
}

static void
evloop(int fd, pid_t chld)
{
	(void)chld;
	struct pollfd fds[1];

	fds[0] = (struct pollfd) {
		.fd = fd,
		.events = POLLIN|POLLERR|POLLHUP,
	};

	for (;;) {
		if (poll(fds, 1, -1) == -1) {
			if (errno == EINTR)
				continue;

			if (kill(chld, SIGTERM) == -1)
				die("kill failed after poll failed");
			die("poll failed");
		}

		/* From XSelectInput(3):
		 *   The XSelectInput function requests that the X
		 *   server report the events associated with the
		 *   specified event mask. Initially, X will not report
		 *   any of these events.
		 *
		 * Thus we should never receive new data on the file
		 * descriptor. If poll(3) returns the socket was most
		 * likely closed by the X server and we can terminate
		 * our program.
		 */
		if (kill(chld, SIGTERM) == -1)
			die("kill failed");
		exit(EXIT_SUCCESS);
	}
}

int
main(int argc, char **argv)
{
	int fd;
	Display *dpy;
	pid_t pid;
	struct sigaction sa;

	if (argc < 2) {
		fprintf(stderr, "Usage: %s CMD...\n", basename(argv[0]));
		return EXIT_FAILURE;
	}

	sa.sa_sigaction = chld;
	sa.sa_flags = SA_SIGINFO | SA_RESTART;
	if (sigemptyset(&sa.sa_mask) == -1)
		die("sigemptyset failed");
	if (sigaction(SIGCHLD, &sa, NULL) == -1)
		die("sigaction failed");

	if (!(dpy = XOpenDisplay(NULL))) {
		fprintf(stderr, "Couldn't open display '%s'\n", XDisplayName(NULL));
		return EXIT_FAILURE;
	}

	if ((fd = XConnectionNumber(dpy)) == -1) {
		fprintf(stderr, "Couldn't obtain fd for X connection\n");
		return EXIT_FAILURE;
	}

	if (fcntl(fd, F_SETFD, FD_CLOEXEC) == -1)
		die("fcntl failed");

	switch ((pid = fork())) {
	case -1:
		die("fork failed");
	case 0:
		memmove(&argv[0], &argv[1], --argc * sizeof(char*));
		argv[argc] = (char*)NULL;

		if (execvp(argv[0], argv) == -1)
			die("execvp failed");
		break;
	default:
		evloop(fd, pid);
		break;
	}

	return EXIT_SUCCESS;
}
