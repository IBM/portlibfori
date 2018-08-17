#ifndef LIBPERFSTAT_H
#define LIBPERFSTAT_H

/* This file describes the structures and constants used by the libperfstat API */

#ifdef __cplusplus
extern "C" {
#endif

#define IDENTIFIER_LENGTH 64                /* length of strings included in the structures */

typedef struct { /* structure element identifier */
    char name[IDENTIFIER_LENGTH]; /* name of the identifier */
} perfstat_id_t;

typedef struct { /* global cpu information */
    int ncpus;                /* number of active logical processors */
    int ncpus_cfg;             /* number of configured processors */
} perfstat_cpu_total_t;

typedef struct { /* Virtual memory utilization */
    u_longlong_t real_total;    /* total real memory (in 4KB pages) */
} perfstat_memory_total_t;


int perfstat_cpu_total(perfstat_id_t *name,
                              perfstat_cpu_total_t* userbuff,
                              int sizeof_userbuff,
                              int desired_number);

int perfstat_memory_total(perfstat_id_t *name,
                                 perfstat_memory_total_t* userbuff,
                                 int sizeof_userbuff,
                                 int desired_number);


#ifdef __cplusplus
}
#endif

#endif /*undef LIBPERFSTAT_H*/

