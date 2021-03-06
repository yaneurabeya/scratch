#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/module.h>
#include <sys/sysctl.h>

#include "test_sysctl.h"

MALLOC_DECLARE(M_MEMGUARD_HELPER);
MALLOC_DEFINE(M_MEMGUARD_HELPER, "memguard_malloc_helper", "Bad memory test malloc zone");

static unsigned long slab_size = PAGE_SIZE;
static long slab_offset;
static unsigned int allocation_attempts = 1;
static int allocate;

static int
sysctl_memguard_malloc_helper_allocate(SYSCTL_HANDLER_ARGS)
{
	char *buf;
	int error, val;
	unsigned int i;

	error = 0;

	if (req->oldptr != NULL || req->oldlen != 0)
		goto end;

	error = sysctl_handle_int(oidp, &val, 0, req);
	if (error || req->newptr == NULL)
		goto end;

	for (i = 0; i < allocation_attempts; i++) {
		buf = malloc(slab_size, M_MEMGUARD_HELPER, M_NOWAIT);
		if (buf == NULL) {
			printf("%s: malloc failed\n", __func__);
			continue;
		}
		buf[slab_offset] = 'a';
		printf("%s: scribbled on malloc buffer @ offset=%ld", __func__,
		    slab_offset);
		free(buf, M_MEMGUARD_HELPER);
	}

end:
	allocate = 0;
	return (0);
}

SYSCTL_ULONG(_test, OID_AUTO, memguard_malloc_helper_slab_size,
    CTLTYPE_ULONG|CTLFLAG_RW, &slab_size, 0,
    "SLAB size to try and allocate memory for with malloc(9)");

SYSCTL_UINT(_test, OID_AUTO, memguard_malloc_helper_allocation_attempts,
    CTLTYPE_UINT|CTLFLAG_RW, &allocation_attempts, 0,
    "Number of attempts for allocating and freeing memory");

SYSCTL_LONG(_test, OID_AUTO, memguard_malloc_helper_slab_offset,
    CTLTYPE_LONG|CTLFLAG_RW, &slab_offset, 0,
    "Virtual offset to seek to in the slab to test memory access protection "
    "support");

SYSCTL_PROC(_test, OID_AUTO, memguard_malloc_helper_allocate,
    CTLTYPE_INT|CTLFLAG_RW, NULL, 0,
    sysctl_memguard_malloc_helper_allocate, "I",
    "Allocate memory according to other related (size, number of attempts, "
    "overallocation)");

static moduledata_t memguard_malloc_helper_moddata = {
	"memguard_malloc_helper",
	_modevent_noop,
	NULL,
};

MODULE_VERSION(memguard_malloc_helper, 1);
DECLARE_MODULE(memguard_malloc_helper, memguard_malloc_helper_moddata,
    SI_SUB_EXEC, SI_ORDER_ANY);
MODULE_DEPEND(memguard_malloc_helper, test_sysctl, 1, 1, 1);
