#ifndef LIBIPERF_H
#define LIBIPERF_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct { 
    int ncpus_online;          /* number of online processors */
    int ncpus_configured;          /* number of configured processors */
} iperfstat_cpu_number_t;


/* Get cpu number */
int iperfstat_cpu_get_number(iperfstat_cpu_number_t* userbuf);

/* Get main storage size(in kilobytes) */
int iperfstat_memory_get_main_storage_size(uint64_t* out);


#ifdef __cplusplus
}
#endif

#endif /*undef LIBIPERF_H*/

