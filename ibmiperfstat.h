#ifndef IBMIPERFSTAT_H
#define IBMIPERFSTAT_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct { 
    int ncpus_onli;          /* number of online processors */
    int ncpus_conf;          /* number of configured processors */
} iperfstat_cpu_number_t;


/* Get cpu number */
int iperfstat_cpu_getNumber(iperfstat_cpu_number_t* userbuf);

/* Get main storage size(in kilobytes) */
int iperfstat_memory_getMSsize();


#ifdef __cplusplus
}
#endif

#endif /*undef IBMIPERFSTAT_H*/

