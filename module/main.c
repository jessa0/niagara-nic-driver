#include "niagara_module.h"
#include "cpld.h"
#include "cpu.h"
#include "pci_cards.inc"

#include <linux/moduleparam.h>

static bool cpld_debug = false;

module_param(cpld_debug, bool, S_IRUGO);
MODULE_PARM_DESC(cpld_debug, "Enable/disable CPLD debug mode");

card_t cards[MAX_CARD];
int maxcard = 0;

static int niagara_major;
static struct class *niagara_class;

static int __init niagara_init(void)
{
	int pci, rc, i;
	dev_t dev = 0;

	MSG_TTY("Version %d.%db%d started",
		NIAGARA_VERSION_MAJOR,
		NIAGARA_VERSION_MINOR,
		NIAGARA_VERSION_BUILD);
	// scan PCI for known devices
	for (pci = 0; pci_cards[pci].vendor; pci++)
		if (supported_card(pci_cards + pci)) {
			struct pci_dev *pci_dev = NULL;
			struct net_device *net_dev;
			int port_in_card = 0;
			for (;; ) {
				pci_dev = pci_get_subsys(pci_cards[pci].vendor, pci_cards[pci].device,
							 pci_cards[pci].ss_vendor, pci_cards[pci].ss_device, pci_dev);
				if (pci_dev == NULL) break;
				if (port_in_card == 0) { // new card
					MSG_TTY("%s (%d ports %s)",
						pci_cards[pci].name,
						pci_cards[pci].num_ports,
						flag2str(pci_cards[pci].flags));
#ifndef CONFIG_IXGBE_INTERACTION
					if ((pci_cards[pci].flags & F_DRIVER) == F_IXGBE)
						MSG_TTY("Warn: LFD support is not compiled");
#endif
					if (maxcard >= MAX_CARD) {
						MSG_TTY("Too many cards, %d max\n", MAX_CARD);
						return -E2BIG;
					}
				}
				cards[maxcard].pci_dev[port_in_card] = pci_dev;

				for_each_netdev(&init_net, net_dev)
				if (net_dev->dev.parent == &pci_dev->dev) {
					cards[maxcard].net_dev[port_in_card] = net_dev;
					break;
				}

				/*
				 * A device driver for DPDK is not netdev. So net_dev[] is null.
				 *
				if (cards[maxcard].net_dev[port_in_card] == NULL) {
					MSG_TTY("Cannot find network device, is Intel driver loaded?");
					return -ENXIO;
				}
				*/

				if (++port_in_card == pci_cards[pci].num_ports) { // card is finished
					cards[maxcard].flags = pci_cards[pci].flags;
					cards[maxcard].num_ports = port_in_card;
					snprintf(cards[maxcard].name, sizeof(cards[maxcard].name), "%s", pci_cards[pci].name);

					// swap segments for N32264 and N42264 NICs as they are physically swapped
					if (strstr(cards[maxcard].name, "N32264") != NULL ||
					  strstr(cards[maxcard].name, "N42264") != NULL) {
						int port_a, port_b;
						for (port_a = 2; port_a < port_in_card; port_a++) {
							if (!(port_a & 0x2)) continue;
							port_b = port_a - 2;
							net_dev = cards[maxcard].net_dev[port_b];
							cards[maxcard].net_dev[port_b] = cards[maxcard].net_dev[port_a];
							cards[maxcard].net_dev[port_a] = net_dev;
							pci_dev = cards[maxcard].pci_dev[port_b];
							cards[maxcard].pci_dev[port_b] = cards[maxcard].pci_dev[port_a];
							cards[maxcard].pci_dev[port_a] = pci_dev;
							DBG("Ports %d and %d were swapped to swap segments", port_b, port_a);
						}
					}

					if (cpld_validate(maxcard)) {
						MSG_TTY("Cannot get response from CPLD of %s", cards[maxcard].name);
						port_in_card = 0;
						continue;
					} else {
						int i = 0; for (; i < port_in_card; i++)
							if (cards[maxcard].net_dev[i] == NULL)
								MSG_TTY(" %d.%d port%d", maxcard, i / 2, i % 2);
							else
								MSG_TTY(" %d.%d port%d %s", maxcard, i / 2, i % 2, cards[maxcard].net_dev[i]->name);
					}

					cards[maxcard].cpld_id = cpld_read(maxcard, ID_R);
					cards[maxcard].oem_id = cpu_read(maxcard, OEM_ID_REG);
					cards[maxcard].product_id = cpu_read(maxcard, PRODUCT_ID_REG);
					cards[maxcard].product_rev = cpu_read(maxcard, PRODUCT_REV_REG);
					cards[maxcard].firmware_rev = cpu_read(maxcard, FW_ID_REG);
					cards[maxcard].secondary_firmware_rev = cpu_read(maxcard, 69);
#ifdef CONFIG_NIAGARA_CPLD_SPI
					cards[maxcard].avr_sig = spi_read_sig(maxcard);
					DBG("avr_sig=%X", cards[maxcard].avr_sig);
#endif
					maxcard++;
					port_in_card = 0;
				}
			}
		}
	MSG_TTY("%d card(s) found", maxcard);
	if (maxcard == 0) return -ENXIO;
//poopulate /dev
	rc = alloc_chrdev_region(&dev, 0, maxcard, "niagara");
	if (rc) return rc;
	niagara_major = MAJOR(dev);
	niagara_class = class_create(THIS_MODULE, "niagara");
	if (IS_ERR(niagara_class)) return PTR_ERR(niagara_class);

	for (i = 0; i < maxcard; i++) {
		struct device *device = device_create(niagara_class, NULL, MKDEV(niagara_major, i), cards + i, "niagara%d", i); //2.6.27
		if (IS_ERR(device)) return PTR_ERR(device);
#ifdef CONFIG_NIAGARA_SYSFS
		rc = sysfs_populate(device, cpld_debug);
		if (rc) return rc;
#endif
	}
#ifdef CONFIG_NIAGARA_PROCFS
	procfs_create();
#endif
	for (i = 0; i < maxcard; i++)
		if (cards[i].pci_dev[0]->dev.driver)
			try_module_get(cards[i].pci_dev[0]->dev.driver->owner);
	// enable LFD functionality
	lfd_init();
	return 0;
}
module_init(niagara_init);


static void __exit niagara_destroy(void)
{
	int i, j;

	for (i = 0; i < maxcard; i++)
		device_destroy(niagara_class, MKDEV(niagara_major, i));
	class_destroy(niagara_class);
	unregister_chrdev_region(MKDEV(niagara_major, 0), maxcard);
#ifdef CONFIG_NIAGARA_PROCFS
	procfs_destroy();
#endif
	// stop all heartbeat and lfd timers
	for (i = 0; i < maxcard; i++)
		for (j = 0; j < cards[i].num_ports / 2; j++) {
			NiagaraSetAttribute(i, j, "heartbeat", 0);
			NiagaraSetAttribute(i, j, "lfd", 0);
		}
	// stop lfd thread
	lfd_deinit();
	for (i = 0; i < maxcard; i++) {
		for (j = 0; j < cards[i].num_ports / 2; j++) {
			if (hw_port[i][j]) iounmap(hw_port[i][j]);
		}
		if (cards[i].pci_dev[0]->dev.driver)
			module_put(cards[i].pci_dev[0]->dev.driver->owner);
	}
	MSG("unloaded");
}
module_exit(niagara_destroy);
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("sergiyg@interfacemasters.com,petrom@interfacemasters.com");
