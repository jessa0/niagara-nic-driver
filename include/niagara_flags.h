#define F_MEDIA 0x0F
#define F_COPPER 0
#define F_FIBER  1

#define F_SPEED 0xF0
#define F_100M  0
#define F_1G    0x10
#define F_10G   0x20
#define F_40G   0x30
#define F_100G  0x40

// interface to CPLD
#define F_SIDE          0x0F00
#define F_FLASH         0x0100
#define F_SDP           0x0200
#define F_ESDP          0x0300
#define F_MDIO          0x0400

// intel driver
#define F_DRIVER        0xF000
#define F_EM            0x1000
#define F_IGB           0x2000
#define F_IXGBE         0x3000

// used chip
#define F_CHIP		0xF0000
#define F_82546GB	0x10000
#define F_82571EB	0x20000
#define F_82576EB	0x30000
#define F_82580EB	0x40000
#define F_82598EB	0x50000
#define F_82599ES	0x60000
#define F_X540		0x70000
#define F_I350		0x80000

// for some NICs port control registers are swapped 
#define F_PCTL_SWAP	0x100000
#define F_PCTL_REGULAR	0x000000
#define F_PCTL_SWAPPED	0x100000

//currently used Itel chips
#define MAC_82546GB     (F_82546GB | F_FLASH | F_EM | F_1G)                 //PCI
#define MAC_82571EB     (F_82571EB | F_SDP  | F_EM | F_1G)                   //PCIe
#define MAC_82576EB     (F_82576EB | F_SDP  | F_IGB | F_1G)
#define MAC_82580EB     (F_82580EB | F_SDP  | F_IGB | F_1G)
#define MAC_82598EB     (F_82598EB | F_SDP  | F_IXGBE | F_10G)
#define MAC_82599ES     (F_82599ES | F_MDIO | F_IXGBE | F_10G)
#define MAC_X540	(F_X540    | F_ESDP | F_IXGBE | F_10G)
#define MAC_I350	(F_I350	   | F_SDP  | F_IGB   | F_1G)

static inline const char *flag2media(unsigned flag)
{
	switch (flag & F_MEDIA) {
	default: return "XXX";
	case F_COPPER: return "COPPER";
	case F_FIBER:  return "FIBER ";
	}
}
static inline const char *flag2speed(unsigned flag)
{
	switch (flag & F_SPEED) {
	default: return "XXX";
	case F_100M:    return "100M";
	case F_1G:      return "1G";
	case F_10G:     return "10G";
	case F_40G:     return "40G";
	case F_100G:    return "100G";
	}
}
static inline const char *flag2side(unsigned flag)
{
	switch (flag & F_SIDE) {
	default: return "XXX";
	case F_FLASH:   return "FLASH";
	case F_SDP:     return "SDP";
	case F_ESDP:    return "ESDP";
	case F_MDIO:    return "MDIO";
	}
}
static inline const char *flag2driver(unsigned flag)
{
	switch (flag & F_DRIVER) {
	default: return "XXX";
	case F_EM:      return "EM";
	case F_IGB:     return "IGB";
	case F_IXGBE:   return "IXGBE";
	}
}
static inline const char *flag2pswap(unsigned flag)
{
	switch (flag & F_PCTL_SWAP) {
	default: return "XXX";
	case F_PCTL_REGULAR: return "REGULAR";
	case F_PCTL_SWAPPED: return "SWAPPED";
	}
}

#define flag2str(f) ({ \
			     static char _str[256]; \
			     snprintf(_str, sizeof(_str), "%s %s %s %s %s", flag2media(f), flag2speed(f), flag2driver(f), flag2side(f), flag2pswap(f)); _str; })
