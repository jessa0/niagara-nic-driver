#include "niagara_module.h"
#include "cpld.h"

#define MDICMD                  0x40000000
#define STCODE_NEW              0x0
#define STCODE_OLD              0x1
#define MDI_ADDR                0x00
#define MDI_WR                  0x01
#define MDI_RD_INC              0x02
#define MDI_RD                  0x03
#define PHY_P0                  0x01
#define PHY_P1                  0x02

#define STCODE_SHIFT            28
#define OPCODE_SHIFT            26
#define PHYADDR_SHIFT           21
#define DEVADDR_SHIFT           16
#define ADDR_SHIFT              0
#define MDI_RD_DAT_SHIFT        16

#define PRTAD                  0x10
#define DEVAD                   ((uintptr_t)(hw_ctrl[hw][1]))


static void cpld_mdio_init(int hw)
{
// looks like MDIO need not initialization
}
uint8_t cpld_mdio_read(int hw, uint8_t addr)
{
	uint32_t temp = 0;
	int i;

	uint8_t *hw_a = hw_ctrl[hw][0];

	cpld_lock(hw);

	/* set MSCA to do Address Cycle Op*/
	temp = (MDICMD | STCODE_NEW << STCODE_SHIFT |
		MDI_ADDR << OPCODE_SHIFT | PRTAD << PHYADDR_SHIFT |
		DEVAD << DEVADDR_SHIFT | (uint16_t)addr << ADDR_SHIFT);

	N22XX_WRITE_REG(hw_a, MSCA, temp);

	/* poll until operation done or 500 usec is up*/
	for (i = 0; i < 10; i++) {
		udelay(50);
		temp = N22XX_READ_REG(hw_a, MSCA);
		if ((temp & MDICMD) == 0)
			break;
	}
	if ((temp & MDICMD) != 0) {
		cpld_unlock(hw);
		return -1;
	}

	/* set MSCA to do a Read Op*/
	temp = (MDICMD | STCODE_NEW << STCODE_SHIFT |
		MDI_RD << OPCODE_SHIFT | PRTAD << PHYADDR_SHIFT |
		DEVAD << DEVADDR_SHIFT | (uint16_t)addr << ADDR_SHIFT);

	N22XX_WRITE_REG(hw_a, MSCA, temp);

	udelay(10);

	/* poll until operation done or 500 usec is up*/
	for (i = 0; i < 10; i++) {
		udelay(50);
		temp = N22XX_READ_REG(hw_a, MSCA);
		if ((temp & MDICMD) == 0)
			break;
	}

	if ((temp & MDICMD) != 0) {
		cpld_unlock(hw);
		return -1;
	}

	/* Get data from MSRWD*/
	temp = N22XX_READ_REG(hw_a, MSRWD);

	cpld_unlock(hw);
	/* shift temp to put in data*/
	return (uint8_t)(temp >> MDI_RD_DAT_SHIFT);
}

static int cpld_mdio_write(int hw, uint8_t addr, uint8_t data)
{
	uint32_t temp = 0;
	int i;

	uint8_t *hw_a = hw_ctrl[hw][0];

	cpld_lock(hw);
	/* set data in MSRWD*/
	N22XX_WRITE_REG(hw_a, MSRWD, (uint16_t)data);

	udelay(10);

	/* set MSCA to do Address Cycle Op*/
	temp = (MDICMD | STCODE_NEW << STCODE_SHIFT |
		MDI_ADDR << OPCODE_SHIFT | PRTAD << PHYADDR_SHIFT |
		DEVAD << DEVADDR_SHIFT | (uint16_t)addr << ADDR_SHIFT);

	N22XX_WRITE_REG(hw_a, MSCA, temp);

	/* poll until operation done or 500 usec is up*/
	for (i = 0; i < 10; i++) {
		udelay(50);
		temp = N22XX_READ_REG(hw_a, MSCA);
		if ((temp & MDICMD) == 0)
			break;
	}
	if ((temp & MDICMD) != 0) {
		cpld_unlock(hw);
		return -1;
	}

	/* set MSCA to do a Write Op*/
	temp = (MDICMD | STCODE_NEW << STCODE_SHIFT |
		MDI_WR << OPCODE_SHIFT | PRTAD << PHYADDR_SHIFT |
		DEVAD << DEVADDR_SHIFT | (uint16_t)addr << ADDR_SHIFT);

	N22XX_WRITE_REG(hw_a, MSCA, temp);

	udelay(10);

	/* poll until operation done or 500 usec is up*/
	for (i = 0; i < 10; i++) {
		udelay(50);
		temp = N22XX_READ_REG(hw_a, MSCA);
		if ((temp & MDICMD) == 0)
			break;
	}
	cpld_unlock(hw);
	if ((temp & MDICMD) != 0)
		return -1;

	return 0;
}
static int cpld_mdio_kick(int card, int segment)
{
	int ret = 0;
	ret = cpld_mdio_write(card, CSR, 0x10 << segment);
	return 0;
}

cpld_functions_t cpld_functions_mdio = {
	cpld_mdio_init, cpld_mdio_read, cpld_mdio_write, cpld_mdio_kick
};
