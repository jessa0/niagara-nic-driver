#include "niagara_module.h"
#include "cpld.h"

static void cpld_flash_init(int hw)
{
	uint32_t eecd = ioread32(hw_ctrl[hw][0] + N22XX_EECD);

	eecd &= ~N22XX_EECD_FWE_MASK;
	eecd |= N22XX_EECD_FWE_EN;
	iowrite32(eecd, hw_ctrl[hw][0] + N22XX_EECD);
}

uint8_t cpld_flash_read(int hw, uint8_t addr)
{
	return ioread8(hw_ctrl[hw][1] + addr);
}

static int cpld_flash_write(int hw, uint8_t addr, uint8_t data)
{
	iowrite8(data, hw_ctrl[hw][1] + addr);
	return 0;
}
static int cpld_flash_kick(int card, int segment)
{
	cpld_flash_write(card, CSR, 0x10 << segment);
	return 0;
}

cpld_functions_t cpld_functions_flash = {
	cpld_flash_init, cpld_flash_read, cpld_flash_write, cpld_flash_kick
};
