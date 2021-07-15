/* 20121223 sergiyg@interfacemasters.com :
 * The problem is: due to different motherboards and BIOSes the order of PCI enumeration is not fixed.
 * For each card we have PCI address space for each port
 * Which two of them are control ports?
 * This is a question.
 * The only solution for now - try all combinations
 */

#include "niagara_module.h"
#include "cpld.h"

unsigned char *hw_port[MAX_CARD][MAX_PORT_PER_CARD];
unsigned char *hw_ctrl[MAX_CARD][2];

static const unsigned char possible_mdio_addr[] = { 0x01, 0x1e };

const cpld_functions_t *cpld_functions[MAX_CARD];


static int cpld_accesible(int hw)
{
	cpld_init(hw);
	cpld_write(hw, AVR_ADDR, 0x02);
	return cpld_read(hw, AVR_ADDR) == 0x02;
}

static int validate_ports(int hw, unsigned char *ctrl0, unsigned char *ctrl1)
{
	hw_ctrl[hw][0] = ctrl0;
	hw_ctrl[hw][1] = ctrl1;
	return cpld_accesible(hw);
}


// for FLASH: check all ports
// for SDP: check all port pairs, straight or reversed
// for MDIO: check all ports with each MDIO addr
int cpld_validate(int hw)
{
	int i, j;

	switch (cards[hw].flags & F_SIDE) {
	case F_FLASH: cpld_functions[hw] = &cpld_functions_flash; break;
	case F_SDP: cpld_functions[hw] = &cpld_functions_sdp; break;
	case F_ESDP: cpld_functions[hw] = &cpld_functions_esdp; break;
	case F_MDIO: cpld_functions[hw] = &cpld_functions_mdio; break;
	default:
		return -1;
	}
	for (i = 0; i < cards[hw].num_ports; i++) {
		hw_port[hw][i] = ioremap(pci_resource_start(cards[hw].pci_dev[i], 0), pci_resource_len(cards[hw].pci_dev[i], 0));
		if (hw_port[hw][i] == NULL)
			MSG_TTY("Cannot ioremap() hw_port - card %d, port %d", hw, i);
	}
	for (i = 0; i < cards[hw].num_ports; i++) {
		switch (cards[hw].flags & F_SIDE) {
		case F_FLASH:
			if (validate_ports(hw, hw_port[hw][i],
					   ioremap(pci_resource_start(cards[hw].pci_dev[i], 2), pci_resource_len(cards[hw].pci_dev[i], 2)))) {
				DBG("control register found at port %d", i);
				return 0;
			}
			continue;
		case F_SDP:
		case F_ESDP:
			if ((i & 1) == 0) continue;
			if (validate_ports(hw, hw_port[hw][i - 1], hw_port[hw][i])) {
				DBG("control registers found as segment %d", i / 2);
				return 0;
			}
			if (validate_ports(hw, hw_port[hw][i], hw_port[hw][i - 1])) {
				DBG("control registers found as segment %d , inversed", i / 2);
				return 0;
			}
			continue;
		case F_MDIO:
			for (j = 0; j < sizeof(possible_mdio_addr) / sizeof(possible_mdio_addr[0]); j++) {
				if (validate_ports(hw, hw_port[hw][i], (unsigned char *)(uintptr_t)possible_mdio_addr[j])) {
					DBG("Detected MDIO port %d addr 0x%02X", i, possible_mdio_addr[j]);
					return 0;
				}
			}
			continue;
		}
	}
	return 1;
}
