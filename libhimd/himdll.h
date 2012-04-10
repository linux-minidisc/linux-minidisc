#include "himd.h"

/* special low-level functions */

int himdll_get_track_info(struct himd * himd, unsigned int idx, struct trackinfo * t, struct himderrinfo * status);
int himdll_strtype(struct himd *himd, unsigned int idx);
int himdll_strlink(struct himd *himd, unsigned int idx);
