#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/module.h>
#include <sys/queue.h>
#include <sys/sysctl.h>
#include <vm/uma.h>

#include "test_sysctl.h"

MALLOC_DECLARE(M_MEMGUARD_HELPER);
MALLOC_DEFINE(M_MEMGUARD_HELPER, "memguard_uma_helper", "Bad memory test uma_zalloc zone");

static uint32_t align;
static long item_offset;
static unsigned long item_size = 256;
static int allocate;
static unsigned int allocation_attempts = 1;

static int
sysctl_memguard_uma_helper_allocate(SYSCTL_HANDLER_ARGS)
{
	char *buf;
	uma_zone_t zone;
	int error, val;
	unsigned int i;

	error = sysctl_handle_int(oidp, &val, 0, req);
	if (error || req->newptr != NULL)
		goto end;

	zone = uma_zcreate("MEMGUARD UMA HELPER", item_size,
 	    NULL, NULL, NULL, NULL, align, UMA_ZONE_VM);
	if (zone == NULL) {
		error = ENOMEM;
		goto end;
	}

	for (i = 0; i < allocation_attempts; i++) {
		buf = uma_zalloc(zone, M_NOWAIT);
		if (buf == NULL) {
			printf("%s: uma_zalloc failed\n", __func__);
			continue;
		}
		buf[item_offset] = 'a';
		uma_zfree(zone, buf);
	}

	uma_zdestroy(zone);

end:
	allocate = 0;
	return (0);
}

SYSCTL_ULONG(_test, OID_AUTO, memguard_uma_helper_item_size,
    CTLTYPE_ULONG|CTLFLAG_RW, &item_size, 0,
    "Size to reserve for a zone keg uma_zalloc(9)");

SYSCTL_UINT(_test, OID_AUTO, memguard_uma_helper_allocation_attempts,
    CTLTYPE_UINT|CTLFLAG_RW, &allocation_attempts, 0,
    "Number of attempts for allocating and freeing memory");

SYSCTL_UINT(_test, OID_AUTO, memguard_uma_helper_align,
    CTLTYPE_UINT|CTLFLAG_RW, &align, 0,
    "Value to pass to uma_zcreate for `align`; see uma.h for more details");

SYSCTL_LONG(_test, OID_AUTO, memguard_uma_helper_item_offset,
    CTLTYPE_LONG|CTLFLAG_RW, &item_offset, 0,
    "Virtual offset to seek to in the item to test memory access protection "
    "support");

SYSCTL_PROC(_test, OID_AUTO, memguard_uma_helper_allocate,
    CTLTYPE_INT|CTLFLAG_RW, NULL, 0,
    sysctl_memguard_uma_helper_allocate, "I",
    "Allocate memory according to other related (size, number of attempts, "
    "overallocation)");

static moduledata_t memguard_uma_helper_moddata = {
	"memguard_uma_helper",
	_modevent_noop,
	NULL,
};

MODULE_VERSION(memguard_uma_helper, 1);
DECLARE_MODULE(memguard_uma_helper, memguard_uma_helper_moddata,
    SI_SUB_EXEC, SI_ORDER_ANY);
MODULE_DEPEND(memguard_malloc_helper, test_sysctl, 1, 1, 1);
