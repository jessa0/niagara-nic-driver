#include "niagara_module.h"

#ifndef CONFIG_IXGBE_INTERACTION
int ixgbe_port_set(struct net_device *netdev, int link_up)
{
	return -EINVAL;
}
#else
#include "ixgbe.h"

// MSDOS TSR technique ;)

static s32 (*orig_check_link)(struct ixgbe_hw *, ixgbe_link_speed *, bool *, bool);

static s32 emulated_check_link(struct ixgbe_hw *hw, ixgbe_link_speed *link_speed, bool *up, bool wait)
{
	s32 rc = orig_check_link(hw, link_speed, up, wait);

	*up = false;
	return rc;
}

int ixgbe_port_set(struct net_device *netdev, int link_up)
{
	struct ixgbe_adapter *adapter = netdev_priv(netdev);
	struct ixgbe_hw *hw;

	if ((adapter == NULL) || (adapter->netdev != netdev)) return -EINVAL;
	hw = &adapter->hw;

	if (hw->mac.ops.check_link == emulated_check_link) {
		if (!link_up) return 0;
		hw->mac.ops.check_link = orig_check_link;
	} else {
		if (link_up) return 0;
		orig_check_link = hw->mac.ops.check_link;
		hw->mac.ops.check_link = emulated_check_link;
	}

	adapter->flags |= IXGBE_FLAG_NEED_LINK_UPDATE;

	return 0;
}
#endif // CONFIG_IXGBE_INTERACTION
