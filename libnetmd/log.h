#ifndef LIBNETMD_TRACE_H
#define LIBNETMD_TRACE_H

typedef enum {
        NETMD_LOG_NONE,

        NETMD_LOG_ERROR,
        NETMD_LOG_WARNING,
        NETMD_LOG_DEBUG,

        NETMD_LOG_ALL
} netmd_loglevel;

/*
  Sets the global trace level
  @param level The trace level
*/
void netmd_set_log_level(netmd_loglevel level);

/*
  Shows a hexdump of binary data
  @param level Trace level
  @param data pointer to binary data to trace
  @param len number of bytes to trace
*/
void netmd_log_hex(netmd_loglevel level, const unsigned char* const data, const size_t len);

/*
  Printf like trace function
  @param level Trace level
  @param fmt printf-like format string
*/
void netmd_log(netmd_loglevel level, const char* const fmt, ...);

#endif
