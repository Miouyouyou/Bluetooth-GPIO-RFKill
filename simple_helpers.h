#ifndef __MYY_ANALYSIS_HELPERS_H
#define __MYY_ANALYSIS_HELPERS_H 1

static inline const char * __restrict myy_bool_str
(bool value)
{
	return value ? "true" : "false";
}

#endif
