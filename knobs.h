#if !defined(KNOBS_H)
#define KNOBS_H

#define KNOB_DESC_SIZE 32
typedef struct {
    int r, c;
    int value;
    char desc[KNOB_DESC_SIZE];
    int id;
    int (*get)(int); // id
    void (*set)(int, int); // id, value
} knob_t;

int knob_update(knob_t * knob);
int knobs_get_width(knob_t * knobs, const int N_KNOBS);
void knobs_position(int rows, int cols, int row_offset, int desc_width_max, knob_t * knobs, const int N_KNOBS);

#endif // KNOBS_H
