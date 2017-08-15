#ifndef __MYY_ANALYSIS_HELPERS_H
#define __MYY_ANALYSIS_HELPERS_H 1

#ifdef DUMBY_THE_EDITOR
#include <generated/autoconf.h>
#endif
#include <linux/platform_device.h>
#include <linux/iommu.h>

static inline const char * __restrict myy_bool_str
(bool value)
{
	return value ? "true" : "false";
}

#endif
