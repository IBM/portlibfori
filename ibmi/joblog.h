#ifndef LIBIBMI_JOBLOG_H
#define LIBIBMI_JOBLOG_H


int joblog_write(const char *_msg_type, const char * _data);
int joblog_printf(const char* _msg_type, const char *format, ...);
int qmhsndpm(const char *_msg_type, const char *_msg);


#endif