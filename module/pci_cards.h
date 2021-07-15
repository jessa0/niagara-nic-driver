typedef struct {
	const char *	name;
	int		vendor, device, ss_vendor, ss_device;
	int		num_ports;
	uint32_t	flags;
} pci_card_t;

static inline int supported_card(pci_card_t *p)
{
#ifndef CONFIG_NIAGARA_SUPPORT_FIBER
	if ((p->flags & F_MEDIA) == F_FIBER) return 0;
#endif
	return 1;
}
