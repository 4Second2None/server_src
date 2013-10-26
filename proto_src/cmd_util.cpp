#include "cmd.h"

#include <stdio.h>

int check_cmd()
{
    if (LE_END >= EL_BEGIN) { fprintf(stderr, "LE_END overflow!\n"); return -1; }
    if (EL_END >= GM_BEGIN) { fprintf(stderr, "EL_END overflow!\n"); return -1; }
    if (GM_END >= MG_BEGIN) { fprintf(stderr, "GM_END overflow!\n"); return -1; }
    if (MG_END >= GE_BEGIN) { fprintf(stderr, "MG_END overflow!\n"); return -1; }
    if (GE_END >= EG_BEGIN) { fprintf(stderr, "GE_END overflow!\n"); return -1; }
    if (EG_END >= GA_BEGIN) { fprintf(stderr, "EG_END overflow!\n"); return -1; }
    if (GA_END >= AG_BEGIN) { fprintf(stderr, "GA_END overflow!\n"); return -1; }
    if (AG_END >= ME_BEGIN) { fprintf(stderr, "AG_END overflow!\n"); return -1; }
    if (ME_END >= EM_BEGIN) { fprintf(stderr, "ME_END overflow!\n"); return -1; }
    if (EM_END >= MA_BEGIN) { fprintf(stderr, "EM_END overflow!\n"); return -1; }
    if (MA_END >= AM_BEGIN) { fprintf(stderr, "MA_END overflow!\n"); return -1; }
    if (AM_END >= CL_BEGIN) { fprintf(stderr, "AM_END overflow!\n"); return -1; }
    if (CL_END >= CG_BEGIN) { fprintf(stderr, "AM_END overflow!\n"); return -1; }
    if (CG_END >= CE_BEGIN) { fprintf(stderr, "AM_END overflow!\n"); return -1; }
    if (CE_END >= CA_BEGIN) { fprintf(stderr, "AM_END overflow!\n"); return -1; }
    if (CA_END >= CM_BEGIN) { fprintf(stderr, "AM_END overflow!\n"); return -1; }
    if (CM_END >= LC_BEGIN) { fprintf(stderr, "AM_END overflow!\n"); return -1; }
    if (LC_END >= GC_BEGIN) { fprintf(stderr, "AM_END overflow!\n"); return -1; }
    if (GC_END >= EC_BEGIN) { fprintf(stderr, "AM_END overflow!\n"); return -1; }
    if (EC_END >= AC_BEGIN) { fprintf(stderr, "AM_END overflow!\n"); return -1; }
    if (AC_END >= MC_BEGIN) { fprintf(stderr, "AM_END overflow!\n"); return -1; }
    return 0;
}
