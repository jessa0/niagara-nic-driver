#ifndef NIAGARA_MODULE_H
#define NIAGARA_MODULE_H

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/pci.h>
#include <linux/netdevice.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/tty.h>
#include <asm/io.h>

#include "niagara_config.h"
#include "niagara_api.h"
#include "niagara_flags.h"
#include "pci_cards.h"

#if  !defined(CONFIG_NIAGARA_SUPPORT_FIBER)
#define is_fiber(card) 0
#elif !defined(CONFIG_NIAGARA_SUPPORT_COPPER)
#define is_fiber(card) 1
#else
#define is_fiber(card) ((cards[card].flags & F_MEDIA) == F_FIBER)
#endif

typedef struct {
	struct pci_dev *	pci_dev[MAX_PORT_PER_CARD];
	struct net_device *	net_dev[MAX_PORT_PER_CARD];
	unsigned char		cpld_id;
	unsigned char		oem_id;
	unsigned char		product_id;
	unsigned char		product_rev;
	unsigned char		firmware_rev;
	unsigned char		secondary_firmware_rev;
	char			name[20];
	int			num_ports;
	uint32_t		flags;
#ifdef CONFIG_NIAGARA_CPLD_SPI
	unsigned		avr_sig;
#endif
} card_t;

static inline const char *_basename(const char *arg)
{
	const char *res = arg;

	while (*arg)
		if (*arg++ == '/') res = arg;
	return res;
}

#ifdef DEBUG
#define DBG(fmt, args ...) printk(KERN_ERR "niagara %s:%d:%s:"fmt "\n", _basename(__FILE__), __LINE__, __FUNCTION__, ## args)
#else
#define DBG(...)
#endif

#define MSG(fmt, args ...) printk(KERN_INFO "niagara "fmt "\n", ## args)
static inline void print_tty(const char *str, int len)
{
	struct tty_struct *tty = current->signal->tty;

	MSG("%s", str);
	if (tty == NULL) return;
	tty->driver->ops->write(tty, str, len);
	tty->driver->ops->write(tty, "\r\n", 2);
}
#define MSG_TTY(fmt, args ...) do { char _str[256]; int _len = scnprintf(_str, sizeof(_str), fmt, ## args); print_tty(_str, _len); } while (0)


#define itoa(x) ({ char _str[64]; snprintf(_str, sizeof(_str), "%X", x); _str; })
#define atoi(x) simple_strtoul(x, NULL, 16)

#include <linux/hardirq.h> //TODO:verify
static inline void msec_delay(int x)
{
	if (in_interrupt()) {
		while (x > 0) {
			udelay(1000);
			x--;
		}
	} else {
		set_current_state(TASK_UNINTERRUPTIBLE);
		schedule_timeout((x * HZ + 999) / 1000);
	}
}


// from main.c
extern pci_card_t pci_cards[]; // pci_cards.inc
extern card_t cards[MAX_CARD];
extern int maxcard;

typedef struct {
	void (*cpld_init)(int card);
	uint8_t (*cpld_read)(int card, uint8_t addr);
	int (*cpld_write)(int card, uint8_t addr, uint8_t data);
	int (*cpld_kick)(int card, int segment);
} cpld_functions_t;

// from cpld_sdp.c
extern cpld_functions_t cpld_functions_sdp;

// from cpld_esdp.c
extern cpld_functions_t cpld_functions_esdp;

// from cpld_mdio.c
extern cpld_functions_t cpld_functions_mdio;

// from cpld_flash.c
extern cpld_functions_t cpld_functions_flash;

//TODO: temporary:
#define N22XX_READ_REG(addr, reg) ioread32((addr) + N22XX_ ## reg)
#define N22XX_WRITE_REG(addr, reg, val) iowrite32(val, (addr) + N22XX_ ## reg)

// from cpld_lock.c
int cpld_lock(int card);
int cpld_unlock(int card);
int cpld_trylock(int card);


//from cpld_validate
extern const cpld_functions_t *cpld_functions[MAX_CARD];
extern unsigned char *hw_port[MAX_CARD][MAX_PORT_PER_CARD];
extern unsigned char *hw_ctrl[MAX_CARD][2];
int cpld_validate(int card);

#define cpld_init(card)                 cpld_functions[card]->cpld_init(card)
#define cpld_read(card, args ...)         cpld_functions[card]->cpld_read(card, args)
#define cpld_write(card, args ...)        cpld_functions[card]->cpld_write(card, args)
#define cpld_kick(card, segment)         cpld_functions[card]->cpld_kick(card, segment)

//from cpu.c
uint8_t cpu_read(int card, uint8_t offset);
uint8_t cpu_write(int card, uint8_t offset, uint8_t value);

// from procfs.c
int procfs_create(void);
int procfs_destroy(void);

// from sysfs.c
int sysfs_populate(struct device *device, bool debug_mode);

// from spi.c
#ifdef CONFIG_NIAGARA_CPLD_SPI
unsigned spi_read_sig(int hw);
int      firmware_upgrade(int card, const uint8_t *data, size_t size);
#endif

//from lfd.c
int lfd_init(void);
int lfd_deinit(void);
int lfd_start(int card, int segment);
int lfd_stop(int card, int segment);
int lfd_get(int card, int segment);

// from port.c
int port_get(int card, int segment, int port);
int port_set(int card, int segment, int port, int value);
int fiber_status_get(int card, int segment, int port);

// from niagara_ixgbe.c
int ixgbe_port_set(struct net_device *ndev, int up);

// from cpld_dev.c
int create_cpld_sysfs(struct kobject *kobj_parent, unsigned n);

#endif
