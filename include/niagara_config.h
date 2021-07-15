#define MAX_CARD                4
#define MAX_PORT_PER_CARD       8
#define DEFAULT_HEARTBEAT_PERIOD 10
#define LFD_PERIOD 1
#define LFD_MAX_CHECK_COUNT       5

#define CONFIG_NIAGARA_SUPPORT_FIBER 1
#define CONFIG_NIAGARA_SUPPORT_COPPER 1

#define CONFIG_NIAGARA_PROCFS   1
#define CONFIG_NIAGARA_SYSFS    1
//#define CONFIG_NIAGARA_CPLD_SPI 1
//#define CONFIG_NIAGARA_FIRMWARE 1  // needs CONFIG_NIAGARA_CPLD_SPI

#ifdef __SIZEOF_LONG__
#if (MAX_CARD > 8 * __SIZEOF_LONG__)
#error Too big MAX_CARD // You can change lock() implementation
#endif
#endif

