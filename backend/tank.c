/**
 * @file tank.c
 * @brief Tank and Water Management implementation using Arrays
 */

#include "tank.h"
#include <stdio.h>
#include <string.h>

static Tank g_tanks[MAX_TANKS];
static int g_tank_count = 0;
static int g_next_tank_id = 1;

static void update_tank_status(Tank *t) {
    if (t->current_level <= 0.001) {
        t->current_level = 0.0;
        snprintf(t->status, sizeof(t->status), "Empty");
    } else {
        double pct = (t->current_level / t->capacity) * 100.0;
        if (pct < 25.0) {
            snprintf(t->status, sizeof(t->status), "Low");
        } else {
            snprintf(t->status, sizeof(t->status), "Normal");
        }
    }
}

void tank_init(void) {
    g_tank_count = 0;
    g_next_tank_id = 1;
    memset(g_tanks, 0, sizeof(g_tanks));
}

void tank_clear_all(void) {
    g_tank_count = 0;
    memset(g_tanks, 0, sizeof(g_tanks));
}

int tank_add(const char *name, double capacity, double initial_level, const char *location) {
    if (g_tank_count >= MAX_TANKS) {
        return -1; /* Array is full */
    }
    if (capacity <= 0.0 || initial_level < 0.0 || initial_level > capacity) {
        return -2; /* Invalid input parameters */
    }

    Tank *t = &g_tanks[g_tank_count];
    t->id = g_next_tank_id++;
    strncpy(t->name, name ? name : "Unnamed Tank", sizeof(t->name) - 1);
    t->name[sizeof(t->name) - 1] = '\0';

    t->capacity = capacity;
    t->current_level = initial_level;

    strncpy(t->location, location ? location : "Main Facility", sizeof(t->location) - 1);
    t->location[sizeof(t->location) - 1] = '\0';

    update_tank_status(t);
    g_tank_count++;
    return t->id;
}

int tank_update_water(int tank_id, double delta_litres) {
    Tank *t = tank_get_by_id(tank_id);
    if (!t) {
        return -1; /* Not found */
    }

    double new_level = t->current_level + delta_litres;
    if (new_level > t->capacity + 0.001) {
        return -2; /* Would exceed capacity */
    }
    if (new_level < -0.001) {
        return -3; /* Negative water level not allowed */
    }

    t->current_level = new_level;
    if (t->current_level < 0.0) t->current_level = 0.0;
    if (t->current_level > t->capacity) t->current_level = t->capacity;

    update_tank_status(t);
    return 0;
}

Tank* tank_get_by_id(int tank_id) {
    for (int i = 0; i < g_tank_count; i++) {
        if (g_tanks[i].id == tank_id) {
            return &g_tanks[i];
        }
    }
    return NULL;
}

int tank_get_all(Tank *out_tanks, int max_out) {
    int count = (g_tank_count < max_out) ? g_tank_count : max_out;
    for (int i = 0; i < count; i++) {
        out_tanks[i] = g_tanks[i];
    }
    return count;
}

int tank_get_count(void) {
    return g_tank_count;
}

int tank_delete(int tank_id) {
    int found_idx = -1;
    for (int i = 0; i < g_tank_count; i++) {
        if (g_tanks[i].id == tank_id) {
            found_idx = i;
            break;
        }
    }
    if (found_idx == -1) {
        return -1;
    }

    /* Shift remaining elements */
    for (int i = found_idx; i < g_tank_count - 1; i++) {
        g_tanks[i] = g_tanks[i + 1];
    }
    g_tank_count--;
    return 0;
}

void tank_get_totals(double *out_total_capacity, double *out_total_available) {
    double cap = 0.0, avail = 0.0;
    for (int i = 0; i < g_tank_count; i++) {
        cap += g_tanks[i].capacity;
        avail += g_tanks[i].current_level;
    }
    if (out_total_capacity) *out_total_capacity = cap;
    if (out_total_available) *out_total_available = avail;
}

void tank_load_defaults(void) {
    tank_init();
    tank_add("North Hill Reservoir", 500000.0, 420000.0, "Zone 1 - High Ground");
    tank_add("Central Basin Tank", 350000.0, 210000.0, "Zone 4 - City Center");
    tank_add("South Valley Storage", 250000.0, 55000.0, "Zone 7 - South End");
    tank_add("East River Balancing Tank", 400000.0, 310000.0, "Zone 10 - Riverside");
}
