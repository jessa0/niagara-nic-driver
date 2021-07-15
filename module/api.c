#include "niagara_module.h"
#include "cpu.h"
#include "cpld.h"

static volatile long unsigned int heartbeat_active[MAX_CARD];
static unsigned char heartbeat_period[MAX_CARD][MAX_PORT_PER_CARD / 2];

static unsigned char rly_sta_addr[] = {
	RLY_STA_ADDR_A,
	RLY_STA_ADDR_B,
	RLY_STA_ADDR_C,
	RLY_STA_ADDR_D,
};
static unsigned char current_mode[] = {
	CURRENT_MODE_REG_A,
	CURRENT_MODE_REG_B,
	CURRENT_MODE_REG_C,
	CURRENT_MODE_REG_D,
};
static unsigned char fiber_default_mode[] = {
	FIBER_DEFAULT_REG_A,
	FIBER_DEFAULT_REG_B,
	FIBER_DEFAULT_REG_C,
	FIBER_DEFAULT_REG_D,
};
static unsigned char copper_default_mode[] = {
	COPPER_DEFAULT_REG_A,
	COPPER_DEFAULT_REG_B,
	COPPER_DEFAULT_REG_C,
	COPPER_DEFAULT_REG_D,
};
static unsigned char power_off_mode[] = {
	POWER_OFF_MODE_REG_A,
	POWER_OFF_MODE_REG_B,
	POWER_OFF_MODE_REG_C,
	POWER_OFF_MODE_REG_D,
};

// ATTRIBUTES STUFF
//version
static int get_version(int card, int segment, unsigned *value)
{
	*value = NIAGARA_VERSION;
	return 0;
}
//maxcard
static int get_maxcard(int card, int segment, unsigned *value)
{
	*value = maxcard;
	return 0;
}
//current_mode
static int bad_current_mode(int card, unsigned char mode)
{
	if (mode == 0x70) return 0;
	if (mode > 5) return 1;
	return 0;
}
static int get_current_mode(int card, int segment, unsigned *value)
{
	*value = cpu_read(card, current_mode[segment]);
	if (is_fiber(card)) {
		*value &= 0x7f;
		if (*value != 0x70) *value >>= 3;
	}
	return 0;
}
static int set_current_mode(int card, int segment, unsigned value)
{
	if (bad_current_mode(card, value)) return -EINVAL;
	if (is_fiber(card)) {
		if (value != 0x70)
			value = (value << 3) | (cpu_read(card, current_mode[segment]) & 3);
		value |= 0x80;
	}
	cpu_write(card, current_mode[segment], value);
	return 0;
}
//default_mode
static int bad_default_mode(int card, unsigned char mode)
{
	if (mode > 5) return 1;
	return 0;
}
static int get_default_mode(int card, int segment, unsigned *value)
{
	if (is_fiber(card))
		*value = (cpu_read(card, fiber_default_mode[segment]) & 0x7f) >> 3;
	else
		*value = cpu_read(card, copper_default_mode[segment]);
	return 0;
}
static int set_default_mode(int card, int segment, unsigned value)
{
	if (bad_default_mode(card, value)) return -EINVAL;
	if (is_fiber(card)) {
		value = 0x83 | (value << 3);
		cpu_write(card, fiber_default_mode[segment], value);
	} else {
		cpu_write(card, copper_default_mode[segment], value);
	}
	return 0;
}
//relay_status
static int get_relay_status(int card, int segment, unsigned *value)
{
	*value = cpu_read(card, rly_sta_addr[segment]);
	if (is_fiber(card)) *value >>= 4;
	return 0;
}
static int set_relay_status(int card, int segment, unsigned value)
{
	return -EPERM;
}
//timeout_scaler
static int get_timeout_scaler(int card, int segment, unsigned *value)
{
	*value = cpu_read(card, segment);
	return 0;
}
static int set_timeout_scaler(int card, int segment, unsigned value)
{
	cpu_write(card, segment, value);
	return 0;
}
//heartbeat
static int get_heartbeat(int card, int segment, unsigned *value)
{
	*value = !!test_bit(segment, heartbeat_active + card);
	return 0;
}
static int set_heartbeat(int card, int segment, unsigned value)
{
  return -ENOTSUPP;
}
//heartbeat_period
static int get_heartbeat_period(int card, int segment, unsigned *value)
{
	*value = heartbeat_period[card][segment];
	return 0;
}
static int set_heartbeat_period(int card, int segment, unsigned value)
{
	heartbeat_period[card][segment] = value;
	return 0;
}
//kick
static int get_kick(int card, int segment, unsigned *value)
{
	return -EPERM;
}
static int set_kick(int card, int segment, unsigned value)
{
	if (value == 0) return 0;
	if (value != 1) return -EINVAL;
	while (cpld_kick(card, segment))
		schedule();
	return 0;
}
//lfd
static int set_lfd(int card, int segment, unsigned value)
{
	switch (value) {
	case 0: lfd_stop(card, segment);
		return 0;
	case 1: lfd_start(card, segment);
		return 0;
	default: return -EINVAL;
	}
}
static int get_lfd(int card, int segment, unsigned *value)
{
	*value = lfd_get(card, segment);
	return 0;
}
// ports
static int get_port0(int card, int segment, unsigned *value)
{
	int status = port_get(card, segment, 0);
	if (is_fiber(card)) status &= fiber_status_get(card, segment, 0);
	*value = status;
	return 0;
}
static int get_port1(int card, int segment, unsigned *value)
{
	int status = port_get(card, segment, 1);
	if (is_fiber(card)) status &= fiber_status_get(card, segment, 1);
	*value = status;
	return 0;
}
static int set_port0(int card, int segment, unsigned value)
{
	if ((value != 1) && (value != 0)) return -EINVAL;
	port_set(card, segment, 0, value);
	return 0;
}
static int set_port1(int card, int segment, unsigned value)
{
	if ((value != 1) && (value != 0)) return -EINVAL;
	port_set(card, segment, 1, value);
	return 0;
}

static int get_power_off_mode(int card, int segment, unsigned *value)
{
	if (is_fiber(card))
		*value = 0x01 & (cpu_read(card, power_off_mode[segment]) >> 4);
	else
		*value = 0x01 & cpu_read(card, power_off_mode[segment]);
	return 0;
}

static int set_power_off_mode(int card, int segment, unsigned value)
{
	unsigned val = cpu_read(card, power_off_mode[segment]);
	if ((value != 1) && (value != 0)) return -EINVAL;
	if (value)
		val |= is_fiber(card) ? 0x10 : 0x01;
	else
		val &= ~(is_fiber(card) ? 0x10 : 0x01);
	cpu_write(card, power_off_mode[segment], val);
	return 0;
}

// readonly card attributes
#define NIAGARA_CARD_ATTRIBUTE(_name, _mode) \
	static int get_ ## _name(int card, int segment, unsigned *value){ \
		*value = cards[card]._name; \
		return 0; \
	}
#include "niagara_card_attributes.h"
#undef NIAGARA_CARD_ATTRIBUTE

// master array of attributes
#define NIAGARA_DRIVER_ATTRIBUTE(_name, _mode) { 0, .name = # _name, get_ ## _name, NULL },
#define NIAGARA_CARD_ATTRIBUTE(_name, _mode) { 1, .name = # _name, get_ ## _name, NULL },
#define NIAGARA_SEGMENT_ATTRIBUTE(_name, _mode) { 2, .name = # _name, get_ ## _name, set_ ## _name },
static const struct  {
	int	depth;
	char	name [32];
	int	(*get_attr)(int card, int segment, unsigned *value);
	int	(*set_attr)(int card, int segment, unsigned value);
} niagara_attributes[] = {
#include "niagara_driver_attributes.h"
#include "niagara_card_attributes.h"
#include "niagara_segment_attributes.h"
#undef NIAGARA_DRIVER_ATTRIBUTE
#undef NIAGARA_CARD_ATTRIBUTE
#undef NIAGARA_SEGMENT_ATTRIBUTE
};

int NiagaraGetAttribute(int card, int segment, const char *name, unsigned *value)
{
	int i;

	if ((card < 0) || (card >= maxcard)) return -EINVAL;
	if ((segment < 0) || (segment >= cards[card].num_ports / 2)) return -EINVAL;
	for (i = 0; i < sizeof(niagara_attributes) / sizeof(niagara_attributes[0]); i++)
		if (strcmp(name, niagara_attributes[i].name) == 0) {
			if (niagara_attributes[i].get_attr == NULL) return -EPERM;
			return niagara_attributes[i].get_attr(card, segment, value);
		}
	return -ENOENT;
}
int NiagaraSetAttribute(int card, int segment, const char *name, unsigned value)
{
	int i;

	if ((card < 0) || (card >= maxcard)) return -EINVAL;
	if ((segment < 0) || (segment >= cards[card].num_ports / 2)) return -EINVAL;
	for (i = 0; i < sizeof(niagara_attributes) / sizeof(niagara_attributes[0]); i++)
		if (strcmp(name, niagara_attributes[i].name) == 0) {
			if (niagara_attributes[i].set_attr == NULL) return -EPERM;
			return niagara_attributes[i].set_attr(card, segment, value);
		}
	return -ENOENT;
}

EXPORT_SYMBOL(NiagaraGetAttribute);
EXPORT_SYMBOL(NiagaraSetAttribute);
