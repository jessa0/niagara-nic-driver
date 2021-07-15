#include "niagara_module.h"

#ifdef CONFIG_NIAGARA_SYSFS

#ifdef CONFIG_NIAGARA_FIRMWARE
#include <linux/firmware.h>
const struct firmware *fw;
#endif

static ssize_t cpld_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf);
static ssize_t cpld_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count);

#define NIAGARA_CPLD_ATTRIBUTE(_name, _mode)	\
static struct kobj_attribute cpld_attribute_ ## _name = { \
	.attr = { .name = # _name, .mode = _mode },\
	.show = cpld_show, .store = cpld_store };
#include "niagara_cpld_attributes.h"
#undef NIAGARA_CPLD_ATTRIBUTE

// special attribute for cpu rw operations
static void delete_cpld_sysfs(unsigned n);

static unsigned cpld_ioreg_data[MAX_CARD];
static unsigned cpld_ioreg_addr[MAX_CARD];

static ssize_t cpld_get_ioreg(struct kobject *kobj, struct kobj_attribute *attr, char *buf);
static ssize_t cpld_set_ioreg(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count);

static struct kobj_attribute cpld_attribute_IOREG = {
	.attr = {
		.name = "IOREG",
		.mode = 0666
	},
	.show = cpld_get_ioreg,
	.store = cpld_set_ioreg,
};

static struct attribute *cpld_attrs[] = {
#define NIAGARA_CPLD_ATTRIBUTE(_name, _mode) &cpld_attribute_ ## _name.attr,
#include "niagara_cpld_attributes.h"
#undef NIAGARA_CPLD_ATTRIBUTE
	&cpld_attribute_IOREG.attr,
	NULL,
};

static struct kobject *cpld[MAX_CARD];

inline static unsigned get_cpld_reg_addr(struct attribute *attrp)
{
	unsigned i;
	for (i = 0; i < sizeof(cpld_attrs) / sizeof(cpld_attrs[0]); i++)
		if (cpld_attrs[i] == attrp) break;
	return i;
}

static ssize_t cpld_get_ioreg(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	unsigned devn;
	devn = MINOR(container_of(kobj->parent, struct device, kobj)->devt);
	return scnprintf(buf, PAGE_SIZE, "%08X\n", ((0x01ff & cpld_ioreg_addr[devn]) << 8) | (0xff & cpld_ioreg_data[devn]));
}

static ssize_t cpld_set_ioreg(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
	unsigned value;
	unsigned devn;
	char *eptr;

	value = simple_strtoul(buf, &eptr, 16);
	if ((*eptr != 0) && (*eptr != '\n')) return -EINVAL;
	if (value & ~0x0101ffff) return -EINVAL;

	devn = MINOR(container_of(kobj->parent, struct device, kobj)->devt);

	cpld_ioreg_addr[devn] = 0x1ff & (value >> 8);
	cpld_ioreg_data[devn] = 0xff & value;

	if (value & (0x01<<24)) {
		DBG("cpld ioreg write operation");
		cpu_write(devn, cpld_ioreg_addr[devn], cpld_ioreg_data[devn]);
	} else {
		DBG("cpld ioreg read operation");
		cpld_ioreg_data[devn] = 0xff & cpu_read(devn, cpld_ioreg_addr[devn]);
	}

	return count;
}

static const struct attribute_group sysfs_cpld_group = {
	.attrs = cpld_attrs,
};

static ssize_t cpld_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	unsigned devn = MINOR(container_of(kobj->parent, struct device, kobj)->devt);
	return snprintf(buf, PAGE_SIZE, "%02X\n", 0xff & cpld_read(devn, get_cpld_reg_addr(&attr->attr)));
}

static ssize_t cpld_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
	unsigned value;
	char *eptr;
	unsigned devn;
	unsigned offset;

	devn = MINOR(container_of(kobj->parent, struct device, kobj)->devt);
	offset = get_cpld_reg_addr(&attr->attr);
	value = simple_strtoul(buf, &eptr, 16);
	if ((*eptr != 0) && (*eptr != '\n')) return -EINVAL;
	if (value >> 8) return -EINVAL;
	cpld_write(devn, offset, value);
	
	return count;
}

int create_cpld_sysfs(struct kobject *kobj_parent, unsigned n)
{
	int rc = 0;

	if (cpld[n]) return -EEXIST;

	cpld[n] = kobject_create_and_add("cpld", kobj_parent);
	if (IS_ERR(cpld[n])) return PTR_ERR(cpld[n]);
	
	rc = sysfs_create_group(cpld[n], &sysfs_cpld_group);
	if (rc) delete_cpld_sysfs(n);
	
	return rc;
}

static void delete_cpld_sysfs(unsigned n)
{
	kobject_put(cpld[n]);
	cpld[n] = NULL;
}

#endif
