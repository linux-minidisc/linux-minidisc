#ifndef LIBNETMD_ERROR_H
#define LIBNETMD_ERROR_H

/**
   Enum with possible error codes the netmd_* functions could return.

   @see netmd_strerror
*/
typedef enum {

    NETMD_NO_ERROR = 0,
    NETMD_NOT_IMPLEMENTED,

    NETMD_USB_OPEN_ERROR,
    NETMD_USB_ERROR,

    NETMD_ERROR,

    NETMD_RESPONSE_TO_SHORT,
    NETMD_RESPONSE_NOT_EXPECTED,

    NETMD_COMMAND_FAILED_NO_RESPONSE,
    NETMD_COMMAND_FAILED_NOT_IMPLEMENTED,
    NETMD_COMMAND_FAILED_REJECTED,
    NETMD_COMMAND_FAILED_UNKNOWN_ERROR,

    NETMD_DES_ERROR

} netmd_error;

/**
   Function that could be used to get a string describing the given error
   number.

   @param error Error number to get the description for.
   @return Pointer to static char buffer to the error description. (Should not
           be freed.)
*/
const char* netmd_strerror(netmd_error error);

#endif
