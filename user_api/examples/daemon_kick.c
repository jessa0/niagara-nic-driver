/* 20131012
 * This is example program.
 * it generates periodic "kick" signals for Niagara card, which are
 * keeping card in ACTIVE (non-BYPASS) state
 * In this example, card is in active while disk space is large enough
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <niagara_api.h>

// You can change following for your needs
#define CHECK_PATH "/"
#define CHECK_SIZE (100llu << 30)  /// hundred gigs
#include <sys/statvfs.h>
static int SystemIsGood(void)
{
	struct statvfs fs;

	if (statvfs(CHECK_PATH, &fs)) return 0;
	return !!(fs.f_bsize * fs.f_bfree > CHECK_SIZE);
}

int card = 0, segment = 0;

static void usage()
{
	fprintf(stderr,
		"Usage: ./daemon_kick [card] [segment]\n");
	exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
	switch (argc) {
	default: usage();
	case 3: segment = atoi(argv[2]);
	case 2: card = atoi(argv[1]);
	case 1:;
	}
// If your dont need following - put here any call to check card existance
// Remove automatic heratbeat (if any). We are master.
	NiagaraSetAttribute(card, segment, "heartbeat", 0);

	fprintf(stderr, "Starting kicks for card %x:%x\n", card, segment);
	fprintf(stderr, "Check free size of %s > %lldG (now %s)\n",
		CHECK_PATH, CHECK_SIZE >> 30,
		SystemIsGood() ? "good" : "bad"
		);
	daemon(0, 0);

// You can change it - for example, add syslog()
	for (;; ) {
		sleep(1);
		NiagaraSetAttribute(card, segment, "kick", SystemIsGood());
	}
	return 0;
}
