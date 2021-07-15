#include "niagara_module.h"
#ifdef CONFIG_NIAGARA_SYSFS


#ifdef CONFIG_NIAGARA_FIRMWARE
#include <linux/firmware.h>
const struct firmware *fw;
#endif

// DRIVER_STUFF
static ssize_t version_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%06X\n", NIAGARA_VERSION);
}
static ssize_t maxcard_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%X\n", maxcard);
}
static ssize_t supported_cards_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	int i, wrote = 0;

	for (i = 0; pci_cards[i].vendor; i++)
		if (supported_card(pci_cards + i)) {
			int portion = scnprintf(buf + wrote, PAGE_SIZE - wrote, "%s\t%d port %s\n", pci_cards[i].name, pci_cards[i].num_ports, flag2str(pci_cards[i].flags));
			wrote += portion;
			if (portion < 10) break;
		}
	return wrote;
}


#define NIAGARA_DRIVER_ATTRIBUTE(_name, _mode) \
	static struct kobj_attribute driver_attribute_ ## _name = { .attr = { .name = # _name, .mode = _mode }, .show = _name ## _show };
#include "niagara_driver_attributes.h"
#undef NIAGARA_DRIVER_ATTRIBUTE
static struct kobj_attribute driver_attribute_supported_cards = { .attr = { .name = "supported_cards", .mode = 0444 }, .show = supported_cards_show };

static struct attribute *driver_attrs[] = {
#define NIAGARA_DRIVER_ATTRIBUTE(_name,		_mode) & driver_attribute_ ## _name.attr,
#include "niagara_driver_attributes.h"
#undef NIAGARA_DRIVER_ATTRIBUTE
	&driver_attribute_supported_cards.attr,
	NULL,                                   /* need to NULL terminate the list of attributes */
};
static const struct attribute_group sysfs_driver_group = {
	.attrs	= driver_attrs,
};

// CARD STUFF
static ssize_t name_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%s\n", cards[MINOR(dev->devt)].name);
}
static ssize_t card_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int rc;
	unsigned value;

	rc = NiagaraGetAttribute(MINOR(dev->devt), 0, attr->attr.name, &value);
	if (rc) return rc;
	return scnprintf(buf, PAGE_SIZE, "%X\n", value);
}
#define NIAGARA_CARD_ATTRIBUTE(_field, _mode) { .attr = { .name = # _field, .mode = _mode }, .show = card_show },
static struct device_attribute device_attributes[] = {
	{ .attr = { "name", .mode = 0444 }, .show = name_show },
#include "niagara_card_attributes.h"
};
#undef NIAGARA_CARD_ATTRIBUTE

// SEGMENT STUFF
static ssize_t segment_show(struct kobject *kobj, struct kobj_attribute *attr,
			    char *buf)
{
	int rc;
	unsigned value;

	rc = NiagaraGetAttribute(MINOR(container_of(kobj->parent, struct device, kobj)->devt), atoi(kobj->name), attr->attr.name, &value);
	if (rc) return rc;
	return scnprintf(buf, PAGE_SIZE, "%X\n", value);
}

static ssize_t segment_store(struct kobject *kobj, struct kobj_attribute *attr,
			     const char *buf, size_t count)
{
	int rc;
	char *eptr;
	unsigned value = simple_strtoul(buf, &eptr, 16);

	if ((*eptr != 0) && (*eptr != '\n')) return -EINVAL;
	rc = NiagaraSetAttribute(MINOR(container_of(kobj->parent, struct device, kobj)->devt), atoi(kobj->name), attr->attr.name, value);
	if (rc) return rc;
	return count;
}

static ssize_t port_name_show(struct kobject *kobj, struct kobj_attribute *attr,
	char *buf)
{
	int card = MINOR(container_of(kobj->parent, struct device, kobj)->devt);
	int segment = atoi(kobj->name);
	int port = 0;
	if (!strncmp(attr->attr.name, "port1_name", 12)) port = 1;
	return scnprintf(buf, PAGE_SIZE, "%s\n", cards[card].net_dev[2*segment + port]->name);
}

#define NIAGARA_SEGMENT_ATTRIBUTE(_name, _mode)	\
	static struct kobj_attribute segment_attribute_ ## _name = { .attr = { .name = # _name, .mode = _mode }, .show = segment_show, .store = segment_store };
#include "niagara_segment_attributes.h"
#undef NIAGARA_SEGMENT_ATTRIBUTE

// port name attributes
static struct kobj_attribute segment_attribute__port0_name =
	{ .attr = { "port0_name", .mode = 0444 }, .show = port_name_show };
static struct kobj_attribute segment_attribute__port1_name =
	{ .attr = { "port1_name", .mode = 0444 }, .show = port_name_show };
static struct attribute *segment_attrs[] = {

#define NIAGARA_SEGMENT_ATTRIBUTE(_name, _mode) & segment_attribute_ ## _name.attr,
#include "niagara_segment_attributes.h"
#undef NIAGARA_SEGMENT_ATTRIBUTE
	&segment_attribute__port0_name.attr,
	&segment_attribute__port1_name.attr,
	NULL,                            /* need to NULL terminate the list of attributes */
};

static const struct attribute_group sysfs_segment_group = {
	.attrs	= segment_attrs,
};

#define CPLD_MEM_SIZE 13
#define CPU_MEM_SIZE 0x7F
static ssize_t cpld_mem_read(struct file *f, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t off, size_t sz)
{
	int i;
	int n = MINOR(container_of(kobj, struct device, kobj)->devt);

	for (i = off; i < off + sz; i++)
		buf[i] = cpld_read(n, i);
	return sz;
}
static ssize_t cpu_mem_read(struct file *f, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t off, size_t sz)
{
	int i;
	int n = MINOR(container_of(kobj, struct device, kobj)->devt);

	for (i = off; i < off + sz; i++)
		buf[i] = cpu_read(n, i);
	return sz;
}
static struct bin_attribute bin_cpld_mem_attr =
{ .attr = { .name = "cpld_mem", .mode = 0444 }, .size = CPLD_MEM_SIZE, .read = cpld_mem_read };
static struct bin_attribute bin_cpu_mem_attr =
{ .attr = { .name = "cpu_mem", .mode = 0444 }, .size = CPU_MEM_SIZE, .read = cpu_mem_read };

int __init sysfs_populate(struct device *device, bool cpld_debug)
{
	int rc, j, a;
	int i = MINOR(device->devt);

	rc = device_create_bin_file(device, &bin_cpld_mem_attr);
	if (rc) return rc;
	rc = device_create_bin_file(device, &bin_cpu_mem_attr);
	if (rc) return rc;

	if (i == 0) {
		rc = sysfs_create_group(device->kobj.parent, &sysfs_driver_group);
		if (rc) return rc;
	}
	for (a = 0; a < sizeof(device_attributes) / sizeof(device_attributes[0]); a++) {
		rc = device_create_file(device, device_attributes + a);
		if (rc) return rc;
	}
	if (cpld_debug) {
		rc = create_cpld_sysfs(&device->kobj, i);
		if (rc) return rc;
	}
	for (j = 0; j < cards[i].num_ports / 2; j++) {
		struct kobject *segment;
		segment = kobject_create_and_add(itoa(j), &device->kobj);
		if (IS_ERR(segment)) return PTR_ERR(segment);
		rc = sysfs_create_group(segment, &sysfs_segment_group);
		if (rc) return rc;
		// symlinks to our Intel eth device
		if (cards[i].net_dev[2 * j]) {
			rc = sysfs_create_link(segment, &cards[i].net_dev[2 * j]->dev.kobj, "device0");
			if (rc) return rc;
		}
		if (cards[i].net_dev[2 * j + 1]) {
			rc = sysfs_create_link(segment, &cards[i].net_dev[2 * j + 1]->dev.kobj, "device1");
			if (rc) return rc;
		}
	}
	rc = sysfs_create_link(device->kobj.parent, &device->kobj, itoa(i));
	if (rc) return rc;
#ifdef CONFIG_NIAGARA_FIRMWARE
	{ char fwname[256];
	  snprintf(fwname, sizeof(fwname), "niagara_%s.bin", cards[i].name);
	  rc = request_firmware(&fw, fwname, device);
	  if (rc == 0) {
		  firmware_upgrade(i, fw->data, fw->size);
		  release_firmware(fw);
	  } else {
		  DBG("Can't find firmware %s, no upgrade", fwname);
	  } }
#endif

	return 0;
}
#endif
