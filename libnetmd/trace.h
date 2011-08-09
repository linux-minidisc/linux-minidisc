#ifndef LIBNETMD_TRACE_H
#define LIBNETMD_TRACE_H

/*
  Sets the global trace level
  @param level The trace level
*/
void netmd_trace_level(int level);

/*
  Shows a hexdump of binary data
  @param level Trace level
  @param data pointer to binary data to trace
  @param len number of bytes to trace
*/
void netmd_trace_hex(int level, char *data, int len);

/*
  Printf like trace function
  @param level Trace level
  @param fmt printf-like format string
*/
void netmd_trace(int level, char *fmt, ...);

#endif
