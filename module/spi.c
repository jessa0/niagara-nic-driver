#include "niagara_module.h"
#include "cpld.h"

#ifdef CONFIG_NIAGARA_CPLD_SPI

#include "spi.h"

#define SPI_PROG_TX cpld_spi_tx
#define SPI_PROG_RX cpld_spi_rx

int cpld_spi_init(int hw)
{
	uint8_t rst_ctrl = 0, cpu_spi = 0;

	lock(hw);

	/* state 0*/
	cpld_tx_two(hw, CSR, 0, 2);

	cpld_rx_two(hw, SPI_CTL, 0, &rst_ctrl);
	cpld_rx_two(hw, SPI_CTL, 2, &cpu_spi);

	/* state 2*/
	rst_ctrl |= SW_RSTN;
	cpld_tx_two(hw, SPI_CTL, 0, rst_ctrl);
	udelay(100);
	cpu_spi &= ~SW_RSTN;
	cpld_tx_two(hw, SPI_CTL, 2, cpu_spi);

	/* state 3*/
	udelay(100);
	rst_ctrl &= ~SW_RSTN;
	cpld_tx_two(hw, SPI_CTL, 0, rst_ctrl);
	udelay(100);
	rst_ctrl |= SW_RSTN;
	cpld_tx_two(hw, SPI_CTL, 0, rst_ctrl);
	/* state 4*/
	udelay(100);
	rst_ctrl &= ~SW_RSTN;
	cpld_tx_two(hw, SPI_CTL, 0, rst_ctrl);
	udelay(100);
	rst_ctrl |= SW_RSTN;
	cpld_tx_two(hw, SPI_CTL, 0, rst_ctrl);

	/* wait for 2 ms before we issue any instructions*/
	msec_delay(20);

	return 0;
}
int cpld_spi_exit(int hw)
{
	uint8_t rst_ctrl = 0, cpu_spi = 0;

	cpld_rx_two(hw, SPI_CTL, 0, &rst_ctrl);
	cpld_rx_two(hw, SPI_CTL, 2, &cpu_spi);

	cpu_spi &= ~SW_RSTN;
	cpld_tx_two(hw, SPI_CTL, 2, cpu_spi);

	/* state 5*/
	rst_ctrl &= ~SW_RSTN;
	cpld_tx_two(hw, SPI_CTL, 0, rst_ctrl);
	udelay(100);

	/* state 6*/
	rst_ctrl |= SW_RSTN;
	cpld_tx_two(hw, SPI_CTL, 0, rst_ctrl);
	udelay(100);

	/* state 7*/
	rst_ctrl &= ~SW_RSTN;
	cpld_tx_two(hw, SPI_CTL, 0, rst_ctrl);
	udelay(100);

	unlock(hw);

	return 0;
}
static uint8_t cpld_spi_rx(int hw)
{
	uint8_t cpld_spi = 0, miso = 0;
	unsigned char i;
	uint8_t rx_data = 0x00;

	cpld_rx_two(hw, SPI_CTL, 2, &cpld_spi);

	/* rx 8 bit data*/
	for (i = 0; i < 8; i++) {
		/* generate rising edge on clk*/
		cpld_spi |= 0x1;
		cpld_tx_two(hw, SPI_CTL, 2, cpld_spi);

		/* wait 2u*/
		udelay(1);

		/* sample */
		/* FIXME: why do we use 3 here?*/
		cpld_rx_two(hw, SPI_CTL, 3, &miso);

		if (miso & 0x2)
			rx_data |= 0x01;
		else
			rx_data &= 0xFE;
		if (i < 7)
			rx_data = rx_data << 1;

		cpld_spi &= ~0x1;
		cpld_tx_two(hw, SPI_CTL, 2, cpld_spi);

		/* wait 2u*/
		udelay(1);
	}
	return rx_data;
}
static void cpld_spi_tx(int hw, uint8_t value)
{
	unsigned char i;
	uint8_t tx_data = value;
	uint8_t cpld_spi = 0;

	/*tx_data = value;*/

	cpld_rx_two(hw, SPI_CTL, 2, &cpld_spi);

	/* TX 8 bit data    */
	for (i = 0; i < 8; i++) {
		/* generate rising edge on clk*/
		if (tx_data & 0x80)
			cpld_spi |= 0x2;
		else
			cpld_spi &= ~0x2;

		/* TX data    */
		cpld_tx_two(hw, SPI_CTL, 2, cpld_spi);
		/* clock data on rising edge*/
		cpld_spi |= 0x1;
		cpld_tx_two(hw, SPI_CTL, 2, cpld_spi);

		/* wait for 2u sec*/
		udelay(1);

		/* generate falling edge on clk*/
		cpld_spi &= ~0x1;
		cpld_tx_two(hw, SPI_CTL, 2, cpld_spi);
		tx_data = tx_data << 1;

		udelay(1);
	}
}






int n22xxp_isp_avr_enter_prog_mode(int hw)
{
	cpld_spi_tx(hw, PROG_ENABLE_1);
	cpld_spi_tx(hw, PROG_ENABLE_2);
	cpld_spi_tx(hw, PROG_ENABLE_3);
	cpld_spi_tx(hw, PROG_ENABLE_4);

	return 0;
}

unsigned spi_read_sig(int hw)
{
	int i;
	unsigned res = 0;

	cpld_spi_init(hw);
	n22xxp_isp_avr_enter_prog_mode(hw);
	for (i = 0; i < 3; i++) {
		SPI_PROG_TX(hw, RD_SIGN_BYTE_1);
		SPI_PROG_TX(hw, RD_SIGN_BYTE_2);
		SPI_PROG_TX(hw, RD_SIGN_BYTE_3 | i);
		res = (res << 8) | SPI_PROG_RX(hw);
	}
	cpld_spi_exit(hw);
	return res;
}
int n22xxp_isp_avr_chip_erase(int hw)
{
	SPI_PROG_TX(hw, CHIP_ERASE_1);
	SPI_PROG_TX(hw, CHIP_ERASE_2);
	SPI_PROG_TX(hw, CHIP_ERASE_3);
	SPI_PROG_TX(hw, CHIP_ERASE_4);
	msec_delay(500);        /* wait at least 18 ms*/

	return 0;
}


int n22xxp_isp_avr_prog_page_load(int hw, uint16_t *value)
{
	uint8_t i;


	for (i = 0; i < 32; i++) {
		/*  load low byte to program memory page */
		SPI_PROG_TX(hw, LOAD_PROG_PAGE_1L);
		SPI_PROG_TX(hw, LOAD_PROG_PAGE_2);
		SPI_PROG_TX(hw, i);
		SPI_PROG_TX(hw, value[i]);

		/*  load high byte to program memory page*/
		SPI_PROG_TX(hw, LOAD_PROG_PAGE_1H);
		SPI_PROG_TX(hw, LOAD_PROG_PAGE_2);
		SPI_PROG_TX(hw, i);
		SPI_PROG_TX(hw, (value[i] >> 8));
	}

	msec_delay(10);

	return 0;
}

int n22xxp_isp_avr_prog_page_write(int hw, uint16_t offset)
{
	/* write program memory page */
	SPI_PROG_TX(hw, WRITE_PROG_PAGE_1);
	SPI_PROG_TX(hw, (offset & 0x0f00) >> 8);
	SPI_PROG_TX(hw, (offset & 0x00e0));
	SPI_PROG_TX(hw, WRITE_PROG_PAGE_4);

	msec_delay(10);

	return 0;
}

int n22xxp_isp_avr_prog_read(int hw, uint16_t offset, uint16_t *value)
{
	/* read the lower byte*/
	SPI_PROG_TX(hw, READ_PROG_1L);
	SPI_PROG_TX(hw, (offset & 0x0f00) >> 8);
	SPI_PROG_TX(hw, (offset & 0x00ff));

	*value = (uint16_t)SPI_PROG_RX(hw);

	/* read the higher byte*/
	SPI_PROG_TX(hw, READ_PROG_1H);
	SPI_PROG_TX(hw, (offset & 0x0f00) >> 8);
	SPI_PROG_TX(hw, (offset & 0x00ff));

	*value |= ((uint16_t)SPI_PROG_RX(hw) << 8);

	return 0;
}

int n22xxp_isp_avr_eeprom_read(int hw, uint16_t offset, uint8_t *value)
{
	SPI_PROG_TX(hw, READ_EPROM_1);
	SPI_PROG_TX(hw, (offset & 0x0100) >> 8);
	SPI_PROG_TX(hw, offset);

	*value = SPI_PROG_RX(hw);

	return 0;
}

int n22xxp_isp_avr_eeprom_write(int hw, uint16_t offset, uint8_t value)
{
	uint8_t tmp = 0;

	SPI_PROG_TX(hw, WRITE_EPROM_1);
	/*SPI_PROG_TX(hw, WRITE_EPROM_2);*/
	SPI_PROG_TX(hw, (offset & 0x0100) >> 8);
	SPI_PROG_TX(hw, offset);
	SPI_PROG_TX(hw, value);

	/* poll until the lower byte have been programmed */
	if (value != 0xff) {
		/*n22xxp_isp_avr_eeprom_read (hw, offset, &tmp);*/
		do
			n22xxp_isp_avr_eeprom_read(hw, offset, &tmp);
		while (tmp != value);
	} else {
		/* wait until the lower byte have been programmed*/
		msec_delay(100);
	}

	return 0;
}

int firmware_upgrade(int card, const uint8_t *data, size_t size)
{
	DBG("Upgrade for card %d size %zd", card, size);
	return 0;
}
#endif
