#include "libcurl_async.h"

/**
 * :print_log
 * basicaly prints the log message to the machine
 * console stderr. those message could monitored later on.
 * @param severity
 * @param func  -   the called function
 * @param message
 **/
void print_log(const char* severity, 
            const char* func, const char* message)
{
  fprintf(stderr, "[%s][%s] when (%s) : %s\n", LUA_ASYNC_HTTP_TITLE, severity, func, message);
}

/**
 * :create_log_by_severity
 * organizing the error / warning messages by thier severity.
 * each severity level gets his own badge name and transferred
 * to print_log function.
 * @param severity
 * @param func
 * @param message
 **/
void create_log_by_severity(unsigned char severity, const char* func, const char* message)
{
  switch (severity)
  {
      case L_DEBUG: print_log("DEBUG", func, message); break;
      case L_INFO: print_log("INFO", func, message); break;
      case L_ERROR: print_log("ERROR", func, message); break;
      case L_FATAL_ERROR: print_log("FATAL ERROR", func, message); break;
  }
}

/**
 * :create_log
 * this function creates a new log message based on the configuration log_level.
 * 
 * severity identifiers are as follow:
 * '1'  -   FATAL ERROR
 * '2'  -   ERROR
 * '3'  -   INFO
 * '4'  -   DEBUG
 * 
 * @param severity
 * @param func
 * @param message
 **/
void create_log(unsigned char severity, const char* func, const char* message)
{
  if (!LOG_LEVEL) return;
  switch (LOG_LEVEL)
  {
    case VERBOSE_V:
      if (severity <= L_ERROR) 
        create_log_by_severity(severity, func, message);
    break;

    case VERBOSE_VV:
      if (severity <= L_INFO) 
        create_log_by_severity(severity, func, message);
    break;

    case VERBOSE_VVV:
      if (severity <= L_DEBUG) 
        create_log_by_severity(severity, func, message);
    break;

    default:
    return;
  }
}

/**
 * :log_debug_info, log_info, log_error, log_fatal_error
 * this function is the entry point for creating a new log message.
 * with parameters. the request is being made by passing the log request 
 * to 'create_log' function.
 * 
 * those functions are work with va_args, which is unlimited number of arguments.
 * the caller syntax should be like: 
 * log_debug_info("function to log", "my number is: %d\n", num).
 * 
 * the below functions are basicaly the same, just with other severity code.
 * 
 * @param func
 * @param message
 * @param ... - va_list
 **/
void log_debug_info(const char* func, const char* format, ...)
{
  va_list arg;
  va_start (arg, format);
  vsnprintf(logger_buffer, LOGGER_BUFFER_SIZE, format, arg);
  va_end(arg);
  create_log(L_DEBUG, func, logger_buffer);
}

void log_info(const char* func, const char* format, ...)
{
  va_list arg;
  va_start (arg, format);
  vsnprintf(logger_buffer, LOGGER_BUFFER_SIZE, format, arg);
  va_end (arg);
  create_log(L_INFO, func, logger_buffer);
}

void log_error(const char* func, const char* format, ...)
{
  va_list arg;
  va_start (arg, format);
  vsnprintf(logger_buffer, LOGGER_BUFFER_SIZE, format, arg);
  va_end (arg);
  create_log(L_ERROR, func, logger_buffer);
}

void log_fatal_error(const char* func, const char* format, ...)
{
  va_list arg;
  va_start (arg, format);
  vsnprintf(logger_buffer, LOGGER_BUFFER_SIZE, format, arg);
  va_end (arg);
  create_log(L_FATAL_ERROR, func, logger_buffer);
}