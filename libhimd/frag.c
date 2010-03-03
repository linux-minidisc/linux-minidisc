#include "himd.h"
#include <string.h>

#define MIN_HOLE 4

int search_hole(struct himd_holelist * holes, int block)
{
    int startidx = 0;
    int endidx = holes->holecnt-1;
    while(startidx != endidx)
    {
        int mididx = (startidx + endidx)/2;
        if(holes->holes[mididx].lastblock < block)
            startidx = mididx+1;
        else
            endidx = mididx;
    }
    return startidx;
}

int himd_find_holes(struct himd * himd, struct himd_holelist * holes, struct himderrinfo * status)
{
    int i;
    holes->holecnt = 1;
    holes->holes[0].firstblock = 0;
    holes->holes[0].lastblock = 0xFFFF;
    for(i = HIMD_FIRST_FRAGMENT;i < HIMD_LAST_FRAGMENT;i++)
    {
        struct fraginfo frag;
        int splitidx;
        if(himd_get_fragment_info(himd, i, &frag, status) < 0)
            return -1;
        if(frag.firstblock == 0 && frag.lastblock == 0)
            continue;	/* unused fragment */
        splitidx = search_hole(holes, frag.firstblock);
        if(frag.firstblock - holes->holes[splitidx].firstblock < MIN_HOLE)
        {
            holes->holes[splitidx].firstblock = frag.lastblock + 1;
            if(holes->holes[splitidx].lastblock < holes->holes[splitidx].firstblock ||
               holes->holes[splitidx].lastblock - holes->holes[splitidx].firstblock < MIN_HOLE)
            {
                /* this hole has been "completely" filled by the fragment */
                memmove(holes->holes + splitidx, holes->holes + splitidx + 1,
                        (holes->holecnt - splitidx - 1) * sizeof(holes->holes[0]));
                holes->holecnt--;
            }
        }
        else
        {
            /* doesn't collapse at the beginning */
            if(holes->holes[splitidx+1].firstblock - frag.lastblock < MIN_HOLE)
                /* but collapses at the end */
                holes->holes[splitidx].lastblock = frag.firstblock - 1;
            else
            {
                memmove(holes->holes + splitidx + 1, holes->holes + splitidx,
                        (holes->holecnt - splitidx) * sizeof(holes->holes[0]));
                holes->holecnt++;
                holes->holes[splitidx].lastblock = frag.firstblock - 1;
                holes->holes[splitidx+1].firstblock = frag.lastblock + 1;
            }
        }
    }
    return 0;
}
