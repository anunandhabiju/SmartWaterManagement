/**
 * @file tank.h
 * @brief Tank and Water Management using Arrays (Data Structure 1)
 *
 * Implements tank storage, capacity bounds checking, water level additions
 * and deductions, and aggregate water metrics using fixed-size arrays.
 */

#ifndef TANK_H
#define TANK_H

#define MAX_TANKS 20

typedef struct {
    int id;
    char name[64];
    double capacity;       /* Maximum capacity in litres */
    double current_level;   /* Current water level in litres */
    char location[64];
    char status[16];       /* "Normal", "Low", "Empty" */
} Tank;

/* Initializes tank storage */
void tank_init(void);

/* Adds a new tank to the array. Returns assigned tank ID, or -1 if full. */
int tank_add(const char *name, double capacity, double initial_level, const char *location);

/* Updates water level for a tank. positive=add water, negative=deduct.
 * Returns: 0 on success, -1 if tank not found, -2 if exceeding capacity, -3 if negative water */
int tank_update_water(int tank_id, double delta_litres);

/* Retrieves a pointer to tank by ID, or NULL if not found */
Tank* tank_get_by_id(int tank_id);

/* Populates array with all tanks. Returns number of tanks copied. */
int tank_get_all(Tank *out_tanks, int max_out);

/* Returns current number of registered tanks */
int tank_get_count(void);

/* Deletes a tank by ID safely. Returns 0 on success, -1 if not found. */
int tank_delete(int tank_id);

/* Computes aggregate capacity and available water across all tanks */
void tank_get_totals(double *out_total_capacity, double *out_total_available);

/* Populates default initial demonstration tanks */
void tank_load_defaults(void);

/* Clears all tank data */
void tank_clear_all(void);

#endif /* TANK_H */
