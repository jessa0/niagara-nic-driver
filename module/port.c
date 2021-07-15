#include "niagara_module.h"
#include "cpld.h"
#include "cpu.h"

//#define READ_FIBER_STATUS_REG(card) cpld_read(card, F_SR)
#define READ_FIBER_STATUS_REG(__card) cpu_read(__card, 0xE0 + F_SR)

int port_get(int card, int segment, int port)
{
	struct net_device *net_dev = cards[card].net_dev[segment * 2 + port];
	if (net_dev == NULL) return -EINVAL;
	return netif_carrier_ok(net_dev);
}

int fiber_status_get(int card, int segment, int port)
{
	if ((cards[card].flags & F_PCTL_SWAP) == F_PCTL_SWAPPED)
		port = port ? 0: 1;
	return (1 << (segment * 2 + port)) & READ_FIBER_STATUS_REG(card) ? 1: 0;
}

static int niagara_phy_read(uint8_t *hw, uint8_t offset, uint16_t *value)
{
	uint32_t val = 0, i;
	val = PHY_RD_OP | PHY_ADDR_MSK | (PHY_REG_MSK & (offset << 16));
	N22XX_WRITE_REG(hw, MDI_CTL, val);
	for (i = 0; i < MAX_PHY_RETRY; i++) {
		udelay(60);
		val = N22XX_READ_REG(hw, MDI_CTL);
		if (val & PHY_RDY_MSK) {
			*value = 0x0FFFF & val;
			return 0;
		}
	}
	return -1;
}
static int niagara_phy_write(uint8_t *hw, uint8_t offset, uint16_t value)
{
	uint32_t val = 0, i;
	val = PHY_WR_OP | PHY_ADDR_MSK | (PHY_REG_MSK & (offset << 16)) | (value & 0x0FFFF);
	N22XX_WRITE_REG(hw, MDI_CTL, val);
	for (i = 0; i < MAX_PHY_RETRY; i++) {
		udelay(60);
		val = N22XX_READ_REG(hw, MDI_CTL);
		if (val & PHY_RDY_MSK)
			return 0;
	}
	return -1;
}

static inline int fiber_port_set(int card, int segment, int port, int up)
{
	uint16_t val;
	unsigned char current_mode[] = {
		CURRENT_MODE_REG_A,
		CURRENT_MODE_REG_B,
		CURRENT_MODE_REG_C,
		CURRENT_MODE_REG_D,
	};
	val = cpu_read(card, current_mode[segment]);
	if ((cards[card].flags & F_PCTL_SWAP) == F_PCTL_SWAPPED)
		port = port ? 0: 1;
	if (val & 0xf0) { // curent mode is set to default mode
		unsigned char default_mode[] = {
			FIBER_DEFAULT_REG_A,
			FIBER_DEFAULT_REG_B,
			FIBER_DEFAULT_REG_C,
			FIBER_DEFAULT_REG_D,
		};
		uint16_t default_val = 0;
		default_val = cpu_read(card, default_mode[segment]);
		if (up)
			default_val |= 1 << port;
		else
			default_val &= ~(1 << port);
		// update default mode and force reset current mode to default
		cpu_write(card, default_mode[segment], default_val);
		cpu_write(card, current_mode[segment], val);
	} else {	
		if (up)
			val |= 1 << port;
		else
			val &= ~(1 << port);
		cpu_write(card, current_mode[segment], val);
	}
	return 0;
}

static inline int copper_port_set(int card, int segment, int port, int up)
{
	uint16_t val = 0;
	uint32_t led = 0;
	niagara_phy_read(hw_port[card][segment * 2 + port], PHY_CTL_REG, &val);
	if (up) {
		val &= ~PHY_PWR_DWN_MSK; /* disable pwr down*/
		led = N22XX_READ_REG(hw_port[card][segment * 2 + port], LED_CTL);
		led &= ~(0xF);
		led |= 0x2;
		N22XX_WRITE_REG(hw_port[card][segment * 2 + port], LED_CTL, led);
	} else {
		val |= PHY_PWR_DWN_MSK; /* set pwr down*/
		led = N22XX_READ_REG(hw_port[card][segment * 2 + port], LED_CTL);
		N22XX_WRITE_REG(hw_port[card][segment * 2 + port], LED_CTL, led | 0x0F);
	}
	niagara_phy_write(hw_port[card][segment * 2 + port], PHY_CTL_REG, val);
	return 0;
}

int port_set(int card, int segment, int port, int up)
{
	if (is_fiber(card)) fiber_port_set(card, segment, port, up);
	if ((cards[card].flags & F_DRIVER) == F_IXGBE)
		return ixgbe_port_set(cards[card].net_dev[segment * 2 + port], up);
	return copper_port_set(card, segment, port, up);
}
