#include "niagara_module.h"
#include <linux/kthread.h>
#include <linux/semaphore.h>

static volatile long unsigned int lfd_active_bmp[MAX_CARD];
static uint8_t lfd_state[MAX_CARD][MAX_PORT_PER_CARD / 2];
static int lfd_count[MAX_CARD][MAX_PORT_PER_CARD / 2];

static struct task_struct *lfd_thread;
static struct semaphore ctl_sem;

static volatile bool lfd_active = false;

static void lfd_update_active(void);
static void lfd_update_segment(int card, int segment);

int lfd_thread_func(void* data)
{
	signed long delay = LFD_PERIOD * HZ;
	DBG("LFD thread started.");
	while (!kthread_should_stop()) {
		//NOTE: double LFD state validation is used here as cpld access
		//control mechanish can also call 'schedule' routine
		if (lfd_active) {
			// update LFD state for all active segments
			lfd_update_active();
		}
		set_current_state(TASK_INTERRUPTIBLE);
		if (lfd_active) {
			schedule_timeout(delay);
		} else {
			schedule();
		}
	}
	DBG("LFD thread stopped.");
	return 0;
}

int lfd_init(void)
{
	sema_init(&ctl_sem, 0);
	lfd_thread = kthread_run(lfd_thread_func, NULL, "niagara-lfd");
	up(&ctl_sem);
	DBG("LFD was initialized");
	return 0;
}

int lfd_deinit(void)
{
	if (lfd_thread) {
		if (down_interruptible(&ctl_sem)) return -EINTR;
		if (lfd_active) lfd_active = false;
		up(&ctl_sem);
		kthread_stop(lfd_thread);
	}
	DBG("LFD was released");
	return 0;
}

int lfd_start(int card, int segment)
{
	if (test_and_set_bit(segment, lfd_active_bmp + card)) return 0;
	if (down_interruptible(&ctl_sem)) return -EINTR;
	if (!lfd_active) {
		lfd_active = true;
		wake_up_process(lfd_thread);
		DBG("LFD thread activated");
	}
	// reset segment state
	lfd_state[card][segment] = 0;
	lfd_count[card][segment] = LFD_MAX_CHECK_COUNT;
	up(&ctl_sem);
	return 0;
}

int lfd_stop(int card, int segment)
{
	if (!test_and_clear_bit(segment, lfd_active_bmp + card)) return 0;
	if (down_interruptible(&ctl_sem)) return -EINTR;
	if (lfd_active)	{
		int active_segments_available = 0, i;
		for (i = 0; i < MAX_CARD; i++) active_segments_available |= lfd_active_bmp[i];
		if (!active_segments_available) {
			lfd_active = false;
			wake_up_process(lfd_thread);
			DBG("LFD thread deactivated");
		}
	}
	// bring up appropriate ports
	port_set(card, segment, 0, 1);
	port_set(card, segment, 1, 1);
	up(&ctl_sem);
	return 0;
}

static void lfd_update_active(void)
{
	int i, j;
	for (i = 0; i < MAX_CARD; i++)
		for (j = 0; j < MAX_PORT_PER_CARD / 2; j++)
			if (test_bit(j, lfd_active_bmp + i))
				lfd_update_segment(i, j);
}

static void lfd_update_segment(int card, int segment)
{
	int port0, port1;

	port0 = port_get(card, segment, 0);
	port1 = port_get(card, segment, 1);
	DBG("%x.%x %1x%1x state=%d count=%d", card, segment, port0, port1, lfd_state[card][segment], lfd_count[card][segment]);
	switch (lfd_state[card][segment]) {
	case 0: if (port0 && port1) {
			lfd_state[card][segment] = 1;
			break;
	}
          // fall through
	case 1:
		if (!port0 && port1) {
			lfd_state[card][segment] = 2;
			port_set(card, segment, 0, 1);
			port_set(card, segment, 1, 0);
		} else if (port0 && !port1) {
			lfd_state[card][segment] = 3;
			port_set(card, segment, 0, 0);
			port_set(card, segment, 1, 1);
		}
		break;
	case 2: if (port0) {
			lfd_state[card][segment] = 4;
			port_set(card, segment, 0, 1);
			port_set(card, segment, 1, 1);
	}
		break;
	case 3: if (port1) {
			lfd_state[card][segment] = 4;
			port_set(card, segment, 0, 1);
			port_set(card, segment, 1, 1);
	}
		break;
	case 4: if (port0 && port1) {
			lfd_state[card][segment] = 1;
			lfd_count[card][segment] = LFD_MAX_CHECK_COUNT;
	} else { if (lfd_count[card][segment]) {
			 lfd_state[card][segment] = 4;
			 lfd_count[card][segment]--;
		 } else {
			 lfd_state[card][segment] = 0;
			 lfd_count[card][segment] = LFD_MAX_CHECK_COUNT;
		 } }
		break;
	default:
		MSG("Internal error - invlid lfd_state[%d][%d]", card, segment);
		lfd_state[card][segment] = 0;
	}
}

int lfd_get(int card, int segment)
{
	return !!test_bit(segment, lfd_active_bmp + card);
}
