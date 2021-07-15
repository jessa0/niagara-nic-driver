NIAGARA_CARD_ATTRIBUTE(cpld_id, 0444)
NIAGARA_CARD_ATTRIBUTE(oem_id, 0444)
NIAGARA_CARD_ATTRIBUTE(product_id, 0444)
NIAGARA_CARD_ATTRIBUTE(product_rev, 0444)
NIAGARA_CARD_ATTRIBUTE(firmware_rev, 0444)
NIAGARA_CARD_ATTRIBUTE(secondary_firmware_rev, 0444)
NIAGARA_CARD_ATTRIBUTE(num_ports, 0444)
NIAGARA_CARD_ATTRIBUTE(flags, 0444)

#ifdef CONFIG_NIAGARA_CPLD_SPI
NIAGARA_CARD_ATTRIBUTE(avr_sig, 0444)
#endif

// name is placed as a file, but it's not attribute
