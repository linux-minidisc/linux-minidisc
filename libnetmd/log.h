#ifndef LIBNETMD_TRACE_H
#define LIBNETMD_TRACE_H

/**
   log level
*/
typedef enum {
        /** Not a log level. Should only be used to filter all log messages so
            that no messages are displayed */
        NETMD_LOG_NONE,

        /** fatal error message */
        NETMD_LOG_ERROR,

        /** warning messages */
        NETMD_LOG_WARNING,

        /** messages to display */
        NETMD_LOG_DEBUG,

        /** Not a log level. Should only be used to display all messages. Should
            be the level with the highest value. */
        NETMD_LOG_ALL
} netmd_loglevel;

/**
   Sets the global log level.

   @param level The maximal log level. All messages with a higher log level are
                filtered out and will not be displayed.
*/
void netmd_set_log_level(netmd_loglevel level);

/**
   Shows a hexdump of binary data.

   @param level Log level of this message.
   @param data Pointer to binary data to display.
   @param len Length of the data.
*/
void netmd_log_hex(netmd_loglevel level, const unsigned char* const data, const size_t len);

/**
   Printf like log function.

   @param level Log level of this message.
   @param fmt printf-like format string
*/
void netmd_log(netmd_loglevel level, const char* const fmt, ...);

#endif
