#include "knobs.h"
#include <string.h>

int knob_update(knob_t * knob)
{
    knob->value = knob->get(knob->id);
    return knob->value;
}

int knobs_get_width(knob_t * knobs, const int N_KNOBS)
{
    int i, desc_width, desc_width_max=0;

    for (i = 0; i < N_KNOBS; i++)
    {
        desc_width = strlen(knobs[i].desc);
        if (desc_width_max < desc_width)
        {
            desc_width_max = desc_width;
        }
    }
    return desc_width_max;
}

void knobs_position(int rows, int cols, int row_offset, int desc_width_max, knob_t * knobs, const int N_KNOBS)
{
    int i, r, c;
    knob_t * k_p;

    r = row_offset;
    c = 2;
    for (i = 0; i < N_KNOBS; i++)
    {
        k_p = &knobs[i];
        if (cols < c + desc_width_max + 2)
        {
            r += 3;
            c = 2;
        }
        k_p->r = r;
        k_p->c = c;
        c += desc_width_max + 1;
    }
}