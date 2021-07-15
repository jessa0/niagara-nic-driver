#include "niagara_module.h"
static volatile long cpld_lock_bits;
int cpld_unlock(int card)
{
	clear_bit(card, &cpld_lock_bits);
	return 0;
}
int cpld_trylock(int card)
{
	return test_and_set_bit(card, &cpld_lock_bits);
}
int cpld_lock(int card)
{
	for (;; ) {
		if (cpld_trylock(card) == 0) return 0;
		if (in_atomic()) return -1;
		schedule();
	}
}
