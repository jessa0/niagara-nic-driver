#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <niagara_api.h>
#include <niagara_flags.h>


#define error(args ...) do { fprintf(stderr, "ERROR "); fprintf(stderr, args); fprintf(stderr, errno ? ":%m\n" : "\n"); exit(EXIT_FAILURE); } while (0)


static inline const char *relay_status2str(unsigned char x)
{
	switch (x) {
	case 1: return "BYPASS";
	case 2: return "ACTIVE";
	case 3: return "NOLINK";
	default: return "XXX";
	}
}

static void usage(void)
{
	printf("Niagara BYPASS NIC support utility Version %d.%db%d\n"
	       "niagara_util [OPTIONS] [card[.segment]]\n"
	       "\t-l\tList of supported cards\n"
	       "\t-S\tCount installedcards\n"
	       "\t-i\tCard/segment information\n"
	       "Segment mode\n"
	       "\t-d<0..5>Set Default mode\n"
	       "\t\t\tHeartBeatPresent NoHeartBeat\n"
	       "\tCopper modes:\n"
	       "\t\t-d0\t  Active\t  Bypass\n"
	       "\t\t-d1\t  Bypass\t  Active\n"
	       "\t\t-d2\t  Active\t  Active\n"
	       "\t\t-d3\t  Active\t  NoLink\n"
	       "\t\t-d4\t  Bypass\t  Bypass\n"
	       "\t\t-d5\t  Nolink\t  NoLink\n"
	       "\tFiber modes:\n"
	       "\t\t-d0\t  Active\t  Active\n"
	       "\t\t-d1\t  Bypass\t  Bypass\n"
	       "\t\t-d2\t  Active\t  Bypass\n"
	       "\t\t-d3\t  Bypass\t  Active\n"
	       "\t\t-d4\t  Active\t  NoLink\n"
	       "\t\t-d5\t  Nolink\t  Nolink\n"
	       "\t-c<0..5>Set Current mode\n"
	       "\t-cd\tSet Current mode to Default\n"
	       "Segment watchdog\n"
	       "\t-t<1..ff>Set timeout (in 0.1 sec)\n"
	       "\t-k\tKick segment\n"
	       "\t-a\tAdd heartbeat\n"
	       "\t-r\tRemove heartbeat\n"
	       "\t-h<1..ff>Heartbeat period (in 0.1 sec)\n"
	       "Link Fault Detection\n"
	       "\t-Q\tEnable LFD\n"
	       "\t-q\tDisable LFD\n"
	       "Power Off mode\n"
	       "\t-P\tBypass\n"
	       "\t-p\tRegular\n"
	       "Read/write to register/memory on local CPU (in CPLD debug mode only!)\n"
	       "\t-g <a>\ta is 1-byte long hexadecimal number -\n\t\taddress on local CPU to read\n"
	       "\t-s <ad>\tad is 2-byte long hexadecimal number -\n\t\tfirst byte is address, second - data to write\n"
	       "Common options\n"
	       "\t-v\tverbose\n"
	       "EVERYTHING IS HEXADECIMAL EVERYWHERE!\n",
	       NIAGARA_VERSION_MAJOR,
	       NIAGARA_VERSION_MINOR,
	       NIAGARA_VERSION_BUILD
	       );
	exit(EXIT_FAILURE);
}

static int verbose = 1;
static unsigned cmd = 0;
#define CMD_CARD_LIST   (1 << 0)
#define CMD_CARD_COUNT  (1 << 1)
#define CMD_INFO        (1 << 2)
#define CMD_TIMEOUT     (1 << 3)
#define CMD_KICK        (1 << 4)
#define CMD_ADD_HEART   (1 << 5)
#define CMD_DEL_HEART   (1 << 6)
#define CMD_PERIOD      (1 << 7)
#define CMD_C           (1 << 8)
#define CMD_D           (1 << 9)
#define CMD_ADD_LFD     (1 << 10)
#define CMD_DEL_LFD     (1 << 11)
#define CMD_PWR_OFF_BP	(1 << 12)
#define CMD_PWR_OFF_RG	(1 << 13)
#define CMD_CPU_READ	(1 << 14)
#define CMD_CPU_WRITE	(1 << 15)

static unsigned maxcard;
static int card, segment;

static int getarg(int *arg)
{
	char *eptr;

	*arg = strtoul(optarg, &eptr, 16);
	if (eptr == optarg) return -1;
	if (*eptr != 0) return -1;
	return 0;
}

int main(int argc, char **argv)
{
	int opt, i, j;
	int timeout_arg, c_arg, d_arg, h_arg, cpu_read_arg, cpu_write_arg;

	while ((opt = getopt(argc, argv, "Slit:d:c:karh:QqPpg:s:v")) != -1)
		switch (opt) {
		default: usage();
		case 'l': cmd |= CMD_CARD_LIST; continue;
		case 'S': cmd |= CMD_CARD_COUNT; continue;
		case 'i': cmd |= CMD_INFO; continue;
		case 't': if (getarg(&timeout_arg)) usage();
			if (timeout_arg > 255) usage();
			cmd |= CMD_TIMEOUT; continue;
		case 'd': if (getarg(&d_arg)) usage();
			cmd |= CMD_D; continue;
		case 'c': if (strcmp(optarg, "d") == 0) c_arg = 0x70;
			else if (getarg(&c_arg)) usage();
			cmd |= CMD_C; continue;
		case 'k': cmd |= CMD_KICK; continue;
		case 'a': cmd |= CMD_ADD_HEART; continue;
		case 'r': cmd |= CMD_DEL_HEART; continue;
		case 'h': if (getarg(&h_arg)) usage();
			cmd |= CMD_PERIOD; continue;
		case 'Q': cmd |= CMD_ADD_LFD; continue;
		case 'q': cmd |= CMD_DEL_LFD; continue;
		case 'P': cmd |= CMD_PWR_OFF_BP; continue;
		case 'p': cmd |= CMD_PWR_OFF_RG; continue;
		case 'g': if (getarg(&cpu_read_arg)) usage();
			if (cpu_read_arg > 255) usage();
			cmd |= CMD_CPU_READ; continue;
		case 's': if (getarg(&cpu_write_arg)) usage();
			if (cpu_write_arg > 0xffff) usage();
			cmd |= CMD_CPU_WRITE; continue;
		case 'v': verbose++; continue;
		}
	if (optind != argc) {
		if (cmd & (CMD_CARD_COUNT | CMD_CARD_LIST)) usage();
		if (sscanf(argv[optind], "%x.%x", &card, &segment) == 2) {
			optind++;
		} else if (sscanf(argv[optind], "%x", &card) == 1) {
			segment = -1;
			optind++;
		} else { usage(); }
	} else {
		card = -1;
		segment = -1;
	}
	if (optind != argc) usage();
	if (cmd == 0) usage();
	{ unsigned version;
	  NiagaraGetAttribute(0, 0, "version", &version);
	  if (NIAGARA_VERSION != version) error("Utility version and driver version mismatch"); }
	if (cmd & CMD_CARD_LIST)
		system("cat /sys/devices/virtual/niagara/supported_cards");
	NiagaraGetAttribute(0, 0, "maxcard", &maxcard);
	if (cmd & CMD_CARD_COUNT) {
		if (verbose <= 1) {
			printf("%X\n", maxcard);
			return 0;
		}
		for (i = 0; i < maxcard; i++) {
			unsigned num_ports, flags;
			char cardname[64];
			NiagaraGetAttribute(i, 0, "num_ports", &num_ports);
			NiagaraGetAttribute(i, 0, "flags", &flags);
			NiagaraGetCharAttribute(i, 0, "name", cardname, sizeof(cardname));
			printf("%X\t%s ( %d ports %s )\n", i, cardname, num_ports, flag2str(flags));
		}
	}
	if ((card != -1) && (card >= maxcard))
		error("Invalid card number , %x max", maxcard - 1);
	for (i = 0; i < maxcard; i++)
		if ((card == -1) || (card == i)) {
			unsigned num_ports;
			NiagaraGetAttribute(i, 0, "num_ports", &num_ports);
			if ((card != -1) && (segment != -1) && (segment >= num_ports / 2))
				error("Invalid segment number , %x.%x max", card, num_ports / 2 - 1);
			if (verbose > 0) {
				if ((cmd & CMD_INFO) && (segment == -1)) {
					unsigned oem_id, cpld_id, product_id, product_rev, firmware_rev, secondary_firmware_rev;
					char cardname[64];
					NiagaraGetAttribute(i, 0, "oem_id", &oem_id);
					NiagaraGetAttribute(i, 0, "cpld_id", &cpld_id);
					NiagaraGetAttribute(i, 0, "product_id", &product_id);
					NiagaraGetAttribute(i, 0, "product_rev", &product_rev);
					NiagaraGetAttribute(i, 0, "firmware_rev", &firmware_rev);
					NiagaraGetAttribute(i, 0, "secondary_firmware_rev", &secondary_firmware_rev);
					NiagaraGetCharAttribute(i, 0, "name", cardname, sizeof(cardname));
					printf("Card #%X %s\n", i, cardname);
					printf("%X\tOEM/CPLD:%02X.%02X\n", i, oem_id, cpld_id);
					printf("%X\tPRODUCT :%02X.%02X\n", i, product_id, product_rev);
					printf("%X\tFIRMWARE:%02X:%02X\n", i, firmware_rev, secondary_firmware_rev);
				}
			}
			if (cmd & CMD_CPU_READ) {
				unsigned cpu_read_data;
				NiagaraSetAttribute(i, j, "cpld/IOREG", cpu_read_arg << 8);
				NiagaraGetAttribute(i, j, "cpld/IOREG", &cpu_read_data);
				printf("card:%d:\taddr:0x%02X\tdata:0x%02X\n", i, cpu_read_arg, 0xff & cpu_read_data);
			}
			if (cmd & CMD_CPU_WRITE)
				NiagaraSetAttribute(i, 0, "cpld/IOREG", 0x1000000 | cpu_write_arg);
			for (j = 0; j < num_ports / 2; j++)
				if ((segment == -1) || (segment == j)) {
					if (cmd & CMD_TIMEOUT)
						NiagaraSetAttribute(i, j, "timeout_scaler", timeout_arg);
					if (cmd & CMD_C)
						NiagaraSetAttribute(i, j, "current_mode", c_arg);
					if (cmd & CMD_D)
						NiagaraSetAttribute(i, j, "default_mode", d_arg);
					if (cmd & CMD_KICK)
						NiagaraSetAttribute(i, j, "kick", 1);
					if (cmd & CMD_PERIOD)
						NiagaraSetAttribute(i, j, "heartbeat_period", h_arg);
					if (cmd & CMD_ADD_HEART)
						NiagaraSetAttribute(i, j, "heartbeat", 1);
					if (cmd & CMD_DEL_HEART)
						NiagaraSetAttribute(i, j, "heartbeat", 0);
					if (cmd & CMD_ADD_LFD)
						NiagaraSetAttribute(i, j, "lfd", 1);
					if (cmd & CMD_DEL_LFD)
						NiagaraSetAttribute(i, j, "lfd", 0);
					if (cmd & CMD_PWR_OFF_BP)
						NiagaraSetAttribute(i, j, "power_off_mode", 1);
					if (cmd & CMD_PWR_OFF_RG)
						NiagaraSetAttribute(i, j, "power_off_mode", 0);
					if (cmd & CMD_INFO) {
						unsigned timeout_scaler;
						unsigned relay_status;
						unsigned current_mode;
						unsigned default_mode;
						unsigned lfd;
						unsigned port0;
						unsigned port1;
						unsigned heartbeat;
						unsigned heartbeat_period;
						unsigned power_off_mode;
						char port0_name[32] = {0};
						char port1_name[32] = {0};
						NiagaraGetAttribute(i, j, "timeout_scaler", &timeout_scaler);
						NiagaraGetAttribute(i, j, "relay_status", &relay_status);
						NiagaraGetAttribute(i, j, "current_mode", &current_mode);
						NiagaraGetAttribute(i, j, "default_mode", &default_mode);
						NiagaraGetAttribute(i, j, "lfd", &lfd);
						NiagaraGetAttribute(i, j, "port0", &port0);
						NiagaraGetAttribute(i, j, "port1", &port1);
#if !(USE_PROCFS == 1)
						NiagaraGetCharAttribute(i, j, "port0_name", port0_name, sizeof(port0_name));
						NiagaraGetCharAttribute(i, j, "port1_name", port1_name, sizeof(port1_name));
#else
						snprintf(port0_name, sizeof(port0_name), "0");
						snprintf(port1_name, sizeof(port1_name), "1");
#endif
						NiagaraGetAttribute(i, j, "heartbeat", &heartbeat);
						NiagaraGetAttribute(i, j, "heartbeat_period", &heartbeat_period);
						NiagaraGetAttribute(i, j, "power_off_mode", &power_off_mode);
						printf("%X.%X\tTIMEOUT:%02X\n", i, j, timeout_scaler);
						printf("%X.%X\tRELAY:%02X", i, j, relay_status);
						if (verbose)
							printf("\t%s\n", relay_status2str(relay_status));
						else
							printf("\n");
						printf("%X.%X\tCURRENT MODE:%02X", i, j, current_mode);
						if (current_mode == 0x70)
							printf("\tDEFAULT\n");
						else
							printf("\n");
						printf("%X.%X\tDEFAULT MODE:%02X\n", i, j, default_mode);
						printf("%X.%X\tLFD:%02X PORT0(%s):%02X PORT1(%s):%02X\n", i, j, lfd, port0_name, port0, port1_name, port1);
						printf("%X.%X\tHEARTBEAT:%02X", i, j, heartbeat);
						if (heartbeat)
							printf("\tPERIOD:%02X\n", heartbeat_period);
						else
							printf("\n");
						printf("%X.%X\tPOWER OFF MODE: %s\n", i, j,
							power_off_mode?"BYPASS":"REGULAR");
					}
				}
		}
	return 0;
}
