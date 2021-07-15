#include "niagara_module.h"
#include "cpld.h"

#define BIT3                            0x8
#define BIT2                            0x4
#define BIT1                            0x2
#define BIT0                            0x1

#define SIZEOFARR(arr) (sizeof(arr)/sizeof(arr[0]))

enum bit { ADO0, ADO1, STB0, STB1, DI0, DI1 };
static const struct {
	int		port;
	uint32_t	datareg, datamask;
	uint32_t	dirreg, dirmask;
	bool		output;
} cpld_pins[] = {
	[ADO1] = { 0, N22XX_CTRL_EXT, N22XX_CTRL_EXT_SDP7_DATA, N22XX_CTRL_EXT, N22XX_CTRL_EXT_SDP7_DIR, 1 },   // IM_DS
	[ADO0] = { 0, N22XX_CTRL_EXT, N22XX_CTRL_EXT_SDP6_DATA, N22XX_CTRL_EXT, N22XX_CTRL_EXT_SDP6_DIR, 1 },   // IM_D4
	[STB0] = { 0, N22XX_CTRL,     N22XX_CTRL_SWDPIN0,	N22XX_CTRL,	N22XX_CTRL_SWDPIO0,	 1 },   // IM_D3
	[STB1] = { 1, N22XX_CTRL,     N22XX_CTRL_SWDPIN0,	N22XX_CTRL,	N22XX_CTRL_SWDPIO0,	 1 },   // IM_D0
	[DI1] =	 { 1, N22XX_CTRL_EXT, N22XX_CTRL_EXT_SDP7_DATA, N22XX_CTRL_EXT, N22XX_CTRL_EXT_SDP7_DIR },      // IM_D2
	[DI0] =	 { 1, N22XX_CTRL_EXT, N22XX_CTRL_EXT_SDP6_DATA, N22XX_CTRL_EXT, N22XX_CTRL_EXT_SDP6_DIR },      // IM_D1
};

static inline int cpld_sdp_set_pin_direction(int hw)
{
	int i, rv = 0;
	uint32_t val;
	void *reg;

	for (i = 0; i < sizeof(cpld_pins) / sizeof(cpld_pins[0]); i++) {
		reg = hw_ctrl[hw][cpld_pins[i].port] + cpld_pins[i].dirreg;
		val = ioread32(reg);
		if (!!(val & cpld_pins[i].dirmask) != cpld_pins[i].output) {
			if (cpld_pins[i].output)
				val |= cpld_pins[i].dirmask;
			else
				val &= ~cpld_pins[i].dirmask;
			if (!rv) {
				rv = -1;
				DBG("SDP pins direction was changed");
			}
		}
		if (rv)	iowrite32(val, reg);
	}

	return rv;
}

static void cpld_sdp_init(int hw) {
	uint8_t value;
	while(cpld_sdp_set_pin_direction(hw)) msleep(50);
	DBG("SDP pins direction was initialized");
	if (strstr(cards[hw].name, "N522") != NULL) {
		// for I350 based NICs only
		cpld_write(hw, 0x01, 0x55);
		value = cpld_read(hw, 0x08);
		cpld_write(hw, 0x08, value | 0x02);
		DBG("I350 power off mode fix was applied");
	}
}

static void inline bit_up(int hw, enum bit bit)
{
	void *reg = hw_ctrl[hw][cpld_pins[bit].port] + cpld_pins[bit].datareg;
	uint32_t val = ioread32(reg);

	val |= cpld_pins[bit].datamask;
	iowrite32(val, reg);
}

static void inline bit_down(int hw, enum bit bit)
{
	void *reg = hw_ctrl[hw][cpld_pins[bit].port] + cpld_pins[bit].datareg;
	uint32_t val = ioread32(reg);

	val &= ~cpld_pins[bit].datamask;
	iowrite32(val, reg);
}

static void inline bit_set(int hw, enum bit bit, uint32_t value)
{
	if (value) bit_up(hw, bit);
	else bit_down(hw, bit);
}

static int inline bit_get(int hw, enum bit bit)
{
	void *reg = hw_ctrl[hw][cpld_pins[bit].port] + cpld_pins[bit].datareg;

	return !!(ioread32(reg) & cpld_pins[bit].datamask);
}

static int cpld_sdp_tx_two(int hw, uint8_t addr, uint8_t bsl, uint8_t dat)
{
	bit_up(hw, STB0);
	bit_up(hw, STB1);
	udelay(1);

	bit_set(hw, ADO1, addr & BIT3);
	bit_set(hw, ADO0, addr & BIT2);
	bit_down(hw, STB0);
	bit_down(hw, STB1);
	udelay(1);

	bit_set(hw, ADO1, addr & BIT1);
	bit_set(hw, ADO0, addr & BIT0);
	bit_up(hw, STB0);
	udelay(1);

	bit_set(hw, ADO1, bsl & BIT1);
	bit_set(hw, ADO0, bsl & BIT0);
	bit_down(hw, STB0);
	udelay(1);

	bit_set(hw, ADO1, dat & BIT1);
	bit_set(hw, ADO0, dat & BIT0);
	bit_up(hw, STB1);
	udelay(1);

	bit_down(hw, STB1);
	udelay(1);

	return 0;
}

static int cpld_sdp_rx_two(int hw, uint8_t addr, uint8_t bsl, uint8_t *val)
{
	bit_up(hw, STB0);
	bit_up(hw, STB1);
	udelay(1);

	bit_set(hw, ADO1, addr & BIT3);
	bit_set(hw, ADO0, addr & BIT2);
	bit_down(hw, STB0);
	bit_down(hw, STB1);
	udelay(1);

	bit_set(hw, ADO1, addr & BIT1);
	bit_set(hw, ADO0, addr & BIT0);
	bit_up(hw, STB0);
	udelay(1);

	bit_set(hw, ADO1, bsl & BIT1);
	bit_set(hw, ADO0, bsl & BIT0);
	bit_down(hw, STB0);
	udelay(1);

	*val = (bit_get(hw, DI1) << 1) | bit_get(hw, DI0);

	return 0;
}

static uint8_t cpld_sdp_read(int hw, uint8_t addr)
{
	uint8_t value, tmp;

	for (;;) {
		cpld_lock(hw);

		cpld_sdp_rx_two(hw, addr, 0, &tmp);  value = tmp & 3;
		cpld_sdp_rx_two(hw, addr, 1, &tmp);  value |= (tmp & 3) << 2;
		cpld_sdp_rx_two(hw, addr, 2, &tmp);  value |= (tmp & 3) << 4;
		cpld_sdp_rx_two(hw, addr, 3, &tmp);  value |= (tmp & 3) << 6;

		if (!cpld_sdp_set_pin_direction(hw))
			break;
		
		cpld_unlock(hw);
		DBG("CPLD SDP write operation was unsuccessfull, trying again...");
		// TODO: check following with I350 specification
		// give hw a chance to complete reset (more than 100 ms)
		msleep(150);
	}

	cpld_unlock(hw);
	return value;
}

static int cpld_sdp_write(int hw, uint8_t addr, uint8_t data)
{
	for (;;) {
		cpld_lock(hw);

		cpld_sdp_tx_two(hw, addr, 0, (data >> 0) & 3);
		cpld_sdp_tx_two(hw, addr, 1, (data >> 2) & 3);
		cpld_sdp_tx_two(hw, addr, 2, (data >> 4) & 3);
		cpld_sdp_tx_two(hw, addr, 3, (data >> 6) & 3);

		if (!cpld_sdp_set_pin_direction(hw))
			break;

		cpld_unlock(hw);
		DBG("CPLD SDP write operation was unsuccessfull, trying again...");
		// TODO: check followin with I350 specification
		// give hw a chance to reset
		msleep(150);
	}
		
	cpld_unlock(hw);
	return 0;
}

static int cpld_sdp_kick(int hw, int segment)
{
	if (cpld_trylock(hw)) return -1;
	
	switch (segment) {
	case 0: cpld_sdp_tx_two(hw, CSR, 2, 1); break;
	case 1: cpld_sdp_tx_two(hw, CSR, 2, 2); break;
	case 2: cpld_sdp_tx_two(hw, CSR, 3, 1); break;
	case 3: cpld_sdp_tx_two(hw, CSR, 3, 2); break;
	}
	
	cpld_unlock(hw);
	
	return cpld_sdp_set_pin_direction(hw);
}

cpld_functions_t cpld_functions_sdp = {
	cpld_sdp_init, cpld_sdp_read, cpld_sdp_write, cpld_sdp_kick
};
