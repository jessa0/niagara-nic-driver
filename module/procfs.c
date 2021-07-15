#include "niagara_module.h"
#ifdef CONFIG_NIAGARA_PROCFS
#include <linux/types.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include <linux/version.h>

typedef struct {
	const char *	name;
	unsigned	mode;
} attribute_t;

#define PROCFS_DEBUG 0

#if PROCFS_DEBUG == 1
#define dbginfo(...) DBG(__VA_ARGS__)
#else
#define dbginfo(...)
#endif

#define NIAGARA_DRIVER_ATTRIBUTE(_name, _mode) { # _name, _mode },
static attribute_t driver_attributes[] = {
#include "niagara_driver_attributes.h"
#undef NIAGARA_DRIVER_ATTRIBUTE
};

#define NIAGARA_CARD_ATTRIBUTE(_name, _mode) { # _name, _mode },
static attribute_t card_attributes[] = {
#include "niagara_card_attributes.h"
#undef NIAGARA_CARD_ATTRIBUTE
};

#define NIAGARA_SEGMENT_ATTRIBUTE(_name, _mode) { # _name, _mode },
static attribute_t segment_attributes[] = {
#include "niagara_segment_attributes.h"
#undef NIAGARA_SEGMENT_ATTRIBUTE
};

static struct proc_dir_entry *niagara_root;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0)

#define SIZEOFARR(a) (sizeof(a) / sizeof(a[0]))

union attr_info {
	void* datap;
	uint32_t data;
};

enum attr_type {
	DRIVER_ATTR = 1,
	CARD_ATTR = 2,
	SEGMENT_ATTR = 3,
};

#define ATTR_TYPE_MSK (0x03<<0)
#define ATTR_CARD_MSK (0x0F<<2)
#define ATTR_SEGM_MSK (0x01<<6)
#define ATTR_INDX_MSK (0xFF<<8)

#define ATTR_SET_TYPE(attrinfo, type) { attrinfo &= ~ATTR_TYPE_MSK; attrinfo |= (type) << 0; }
#define ATTR_SET_CARD(attrinfo, card) { attrinfo &= ~ATTR_CARD_MSK; attrinfo |= (card) << 2; }
#define ATTR_SET_SEGM(attrinfo, segm) { attrinfo &= ~ATTR_SEGM_MSK; attrinfo |= (segm) << 6; }
#define ATTR_SET_INDX(attrinfo, indx) { attrinfo &= ~ATTR_INDX_MSK; attrinfo |= (indx) << 8; }

#define ATTR_GET_TYPE(attrinfo) ((attrinfo & ATTR_TYPE_MSK) >> 0)
#define ATTR_GET_CARD(attrinfo) ((attrinfo & ATTR_CARD_MSK) >> 2)
#define ATTR_GET_SEGM(attrinfo) ((attrinfo & ATTR_SEGM_MSK) >> 6)
#define ATTR_GET_INDX(attrinfo) ((attrinfo & ATTR_INDX_MSK) >> 8)

static ssize_t proc_read(struct file *filp, char __user *buf, size_t count,  loff_t *offp);
static ssize_t proc_write(struct file *filp, const char __user *buf, size_t count, loff_t *offp);
static ssize_t proc_read_supported_cards(struct file *filp, char __user *buf, size_t count, loff_t *offp);
static ssize_t proc_read_name(struct file *filp, char __user *buf, size_t count, loff_t *offp);

static const struct file_operations proc_fops_ro = {
	.owner = THIS_MODULE,
	.read  = proc_read,
};

static const struct file_operations proc_fops_rw = {
	.owner = THIS_MODULE,
	.read  = proc_read,
	.write = proc_write,
};

static const struct file_operations proc_fops_get_name  = {
	.owner = THIS_MODULE,
	.read = proc_read_name,
};

static const struct file_operations proc_fops_get_cards_list = {
	.owner = THIS_MODULE,
	.read = proc_read_supported_cards,
};

static inline int get_attr_index(const char* attr_name, const attribute_t attributes[], int count)
{
	int i, index = -1;
	for (i = 0; i < count; i++)
		if (!strcmp(attr_name, attributes[i].name)) {
			index = i;
			break;
		}
	dbginfo("attribute '%s' index is %d", attributes[index].name, index);
	return index;
}

static void init_attr_info(
	union attr_info *attrinfop,
	enum attr_type type,
	int card,
	int segment,
	const char *attr_name)
{
	int index = 0;

	ATTR_SET_TYPE(attrinfop->data, type);
	ATTR_SET_CARD(attrinfop->data, card);
	ATTR_SET_SEGM(attrinfop->data, segment);

	switch (type) {
	case DRIVER_ATTR:
		index = get_attr_index(
			attr_name, driver_attributes, SIZEOFARR(driver_attributes));
		break;
	case CARD_ATTR:
		index = get_attr_index(
			attr_name, card_attributes, SIZEOFARR(card_attributes));
		break;
	case SEGMENT_ATTR:
		index = get_attr_index(
			attr_name, segment_attributes, SIZEOFARR(segment_attributes));
		break;
	default:
		dbginfo("invalid attribute type");
		break;
	}

	ATTR_SET_INDX(attrinfop->data, index);
}

static inline const char* get_attr_name(union attr_info attrinfo)
{
	switch (ATTR_GET_TYPE(attrinfo.data)) {
	case DRIVER_ATTR:
		return driver_attributes[ATTR_GET_INDX(attrinfo.data)].name;
	case CARD_ATTR:
		return card_attributes[ATTR_GET_INDX(attrinfo.data)].name;
	case SEGMENT_ATTR:
		return segment_attributes[ATTR_GET_INDX(attrinfo.data)].name;
	default:
		return NULL;
	}
}

static inline void create_attribute(attribute_t *attributep, struct proc_dir_entry *parent, const struct file_operations *fops, union attr_info attrinfo)
{
	proc_create_data(attributep->name, attributep->mode, parent, fops, attrinfo.datap);
}

static ssize_t proc_read_supported_cards(struct file *filp, char __user *buf, size_t count, loff_t *offp)
{
	int i, bytes_wrote = 0;

	if (*offp > 0) return 0;

	for (i = 0; pci_cards[i].vendor; i++)
		if (supported_card(pci_cards + i)) {
			int portion = scnprintf(buf + bytes_wrote, count - bytes_wrote, "%s\t%d port %s\n", pci_cards[i].name, pci_cards[i].num_ports, flag2str(pci_cards[i].flags));
			bytes_wrote += portion;
			if (portion < 10) break;
		}

	*offp = bytes_wrote;
	return bytes_wrote;
}

static ssize_t proc_read_name(struct file *filp, char __user *buf, size_t count, loff_t *offp)
{
	ssize_t bytes_read = 0;
	char *data = PDE_DATA(file_inode(filp));
	if (*offp > 0) return 0; // EOF reached
	bytes_read = scnprintf(buf, count, "%s\n", (char *)data);
	*offp = bytes_read;
	return bytes_read;
}

int __init procfs_create(void)
{
	int i, j, a;
	union attr_info attrinfo = { .data = 0 };

	niagara_root = proc_mkdir("niagara", NULL);
	proc_create("supported_cards", 0444, niagara_root, &proc_fops_get_cards_list);

	for (a = 0; a < SIZEOFARR(driver_attributes); a++) {
		init_attr_info(&attrinfo, DRIVER_ATTR, 0, 0, driver_attributes[a].name);
		create_attribute(driver_attributes + a, niagara_root, &proc_fops_ro, attrinfo);
	}

	for (i = 0; i < maxcard; i++) {
		struct proc_dir_entry *card_root = proc_mkdir(itoa(i), niagara_root);
		proc_create_data("name", 0444, card_root, &proc_fops_get_name, cards[i].name);

		for (a = 0; a < SIZEOFARR(card_attributes); a++) {
			init_attr_info(&attrinfo, CARD_ATTR, i, 0, card_attributes[a].name);
			create_attribute(card_attributes + a, card_root, &proc_fops_ro, attrinfo);
		}

		for (j = 0; j < cards[i].num_ports / 2; j++) {
			struct proc_dir_entry *segment_root = proc_mkdir(itoa(j), card_root);
			for (a = 0; a < SIZEOFARR(segment_attributes); a++) {
				init_attr_info(&attrinfo, SEGMENT_ATTR, i, j, segment_attributes[a].name);
				create_attribute(segment_attributes + a, segment_root, &proc_fops_rw, attrinfo);
			}
		}
	}
	return 0;
}

ssize_t proc_read(struct file *filp, char __user *buf, size_t count, loff_t *offp)
{
	ssize_t bytes_wrote = 0;
	int rc, card = 0, segment = 0;
	unsigned value;

	union attr_info attrinfo = { .datap = PDE_DATA(file_inode(filp)) };

	if (*offp > 0) return 0;

	card = ATTR_GET_CARD(attrinfo.data);
	segment = ATTR_GET_SEGM(attrinfo.data);

	rc = NiagaraGetAttribute(card, segment, get_attr_name(attrinfo), &value);
	if (rc) return rc;

	bytes_wrote = scnprintf(buf, count, "%X\n", value);
	*offp = bytes_wrote;

	return bytes_wrote;
}

ssize_t proc_write(struct file *filp, const char __user *buf, size_t count, loff_t *offp)
{
	int rc, card = 0, segment = 0;
	char cmd[64], *eptr;
	unsigned value;

	union attr_info attrinfo = { .datap = PDE_DATA(file_inode(filp)) };

	if (count > sizeof(cmd)) count = sizeof(cmd);
	if (copy_from_user(cmd, buf, count)) return -EFAULT;
	value = simple_strtoul(cmd, &eptr, 16);
	if (eptr == cmd) return -EINVAL;
	if ((*eptr != 0) && (*eptr != '\n')) return -EINVAL;

	card = ATTR_GET_CARD(attrinfo.data);
	segment = ATTR_GET_SEGM(attrinfo.data);

	rc = NiagaraSetAttribute(card, segment, get_attr_name(attrinfo), value);
	if (rc) return rc;
	return count;
}

#else

static int proc_read(char *buf, char **start, off_t off, int count, int *eof, void *data)
{
	int rc, card = 0, segment = 0;
	unsigned value;
	struct proc_dir_entry *param = (struct proc_dir_entry *)data; // data points to our proc_dir_entry

	if (off > 0) return 0;
	if (strcmp(param->parent->name, "niagara")) {
		if (strcmp(param->parent->parent->name, "niagara")) { // segment
			segment = atoi(param->parent->name);
			card = atoi(param->parent->parent->name);
		} else { //card
			card = atoi(param->parent->name);
		}
	}
	rc = NiagaraGetAttribute(card, segment, param->name, &value);
	if (rc) return rc;
	return scnprintf(buf, count, "%X\n", value);
}
static int proc_write(struct file *file, const char *buffer, unsigned long count, void *data)
{
	int rc, card = 0, segment = 0;
	char cmd[64], *eptr;
	unsigned value;
	struct proc_dir_entry *param = (struct proc_dir_entry *)data; // data points to our proc_dir_entry

	if (count > sizeof(cmd)) count = sizeof(cmd);
	if (copy_from_user(cmd, buffer, count)) return -EFAULT;
	value = simple_strtoul(cmd, &eptr, 16);
	if (eptr == cmd) return -EINVAL;
	if ((*eptr != 0) && (*eptr != '\n')) return -EINVAL;
	if (strcmp(param->parent->name, "niagara")) {
		if (strcmp(param->parent->parent->name, "niagara")) { // segment
			segment = atoi(param->parent->name);
			card = atoi(param->parent->parent->name);
		} else { //card
			card = atoi(param->parent->name);
		}
	}
	rc = NiagaraSetAttribute(card, segment, param->name, value);
	if (rc) return rc;
	return count;
}
static int proc_read_supported_cards(char *buf, char **start, off_t off, int count, int *eof, void *data)
{
	int i, wrote = 0;

	if (off > 0) return 0;
	for (i = 0; pci_cards[i].vendor; i++)
		if (supported_card(pci_cards + i)) {
			int portion = scnprintf(buf + wrote, count - wrote, "%s\t%d port %s\n", pci_cards[i].name, pci_cards[i].num_ports, flag2str(pci_cards[i].flags));
			wrote += portion;
			if (portion < 10) break;
		}
	return wrote;
}
static int proc_read_name(char *buf, char **start, off_t off, int count, int *eof, void *data)
{
	if (off > 0) return 0;
	return scnprintf(buf, count, "%s\n", (char *)data);
}

static struct proc_dir_entry *create_attribute(attribute_t *a, struct proc_dir_entry *parent, read_proc_t *read, write_proc_t *write)
{
	struct proc_dir_entry *entry;

	entry = create_proc_entry(a->name, a->mode, parent);
	if (entry == NULL) return NULL;
	entry->read_proc = read;
	entry->write_proc = write;
	entry->data = entry;
	entry->nlink = 1;
	return entry;
}

int __init procfs_create(void)
{
	int i, j, a;

	niagara_root = proc_mkdir("niagara", NULL);
	create_proc_read_entry("supported_cards", 0444, niagara_root, proc_read_supported_cards, NULL);
	for (a = 0; a < sizeof(driver_attributes) / sizeof(driver_attributes[0]); a++)
		create_attribute(driver_attributes + a, niagara_root, proc_read, NULL);
	for (i = 0; i < maxcard; i++) {
		struct proc_dir_entry *card_root = proc_mkdir(itoa(i), niagara_root);
		create_proc_read_entry("name", 0444, card_root, proc_read_name, cards[i].name);
		for (a = 0; a < sizeof(card_attributes) / sizeof(card_attributes[0]); a++)
			create_attribute(card_attributes + a, card_root, proc_read, NULL);
		for (j = 0; j < cards[i].num_ports / 2; j++) {
			struct proc_dir_entry *segment_root = proc_mkdir(itoa(j), card_root);
			for (a = 0; a < sizeof(segment_attributes) / sizeof(segment_attributes[0]); a++)
				create_attribute(segment_attributes + a, segment_root, proc_read, proc_write);
		}
	}
	return 0;
}

#endif

int procfs_destroy(void)
{
	int i, j, a;
	char str[256];

	for (i = 0; i < maxcard; i++) {
		for (j = 0; j < cards[i].num_ports / 2; j++) {
			for (a = 0; a < sizeof(segment_attributes) / sizeof(segment_attributes[0]); a++) {
				snprintf(str, sizeof(str), "niagara/%X/%X/%s", i, j, segment_attributes[a].name);
				remove_proc_entry(str, NULL);
			}
			snprintf(str, sizeof(str), "niagara/%X/%X", i, j);
			remove_proc_entry(str, NULL);
		}
		for (a = 0; a < sizeof(card_attributes) / sizeof(card_attributes[0]); a++) {
			snprintf(str, sizeof(str), "niagara/%X/%s", i, card_attributes[a].name);
			remove_proc_entry(str, NULL);
		}
		snprintf(str, sizeof(str), "niagara/%X/name", i);
		remove_proc_entry(str, NULL);
		snprintf(str, sizeof(str), "niagara/%X", i);
		remove_proc_entry(str, NULL);
	}
	for (a = 0; a < sizeof(driver_attributes) / sizeof(driver_attributes[0]); a++) {
		snprintf(str, sizeof(str), "niagara/%s", driver_attributes[a].name);
		remove_proc_entry(str, NULL);
	}
	remove_proc_entry("supported_cards", niagara_root);
	remove_proc_entry("niagara", NULL);
	return 0;
}

#endif
