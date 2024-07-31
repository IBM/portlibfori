
#ifndef LIBIBMI_QSYGETPH_H
#define LIBIBMI_QSYGETPH_H
int validate_pw(char *_username, char *_pw);
int qsygetph(char *_username, char *_pw, char *_handle_buf);
int qsyrlsph(char *_handle_buf);
int qsygetph_np(char *_username, char *_handle_buf);

#endif