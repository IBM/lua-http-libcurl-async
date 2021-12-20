#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <curl/multi.h>

#define DEFAULT_MAX 10                    /* default MAX number of simultaneous transfers               */
#define DEFAULT_REQUEST_TIMEOUT 8L        /* default request timeout incase response being delayed      */
#define MILLISECONDS 1000                 /* milliseconds                                               */
#define DEFAULT_REQUEST_EXPECTATIONS 0L   /* default request header expectations aka verifications      */
#define PP_CERT_TYPE "PEM"
#define LOG_LEVEL 2

#define LOGGER_BUFFER_SIZE 128
#define TBL_KEY_SZ 256
#define TBL_VAL_SZ 1024
#define HEADER_SPACING 2
#define LUA_ASYNC_HTTP_TITLE "LUA_ASYNC_HTTP_LIB"
#define DISABLE_EXPECT_100_CONTINUE "Expect:"

/* ============================================= OBJECTS ============================================= */

typedef struct {
  char* ptr;
  size_t len;
} string;

typedef struct {
  string* headers;
  struct  curl_slist* slist;
  size_t  count;
} header;

typedef struct {
  string  request_key;                    /* the outer request id                                       */
  string  url;                            /* request url                                                */
  string  response_body;                  /* response body                                              */
  string  response_headers;               /* response headers                                           */
  long    response_status;                /* response status                                            */
  char    response_err[CURL_ERROR_SIZE];  /* detailed response error                                    */

  header  header_fields;                  /* request headers                                            */
  string  request_method;                 /* request method                                             */
  string  post_params;                    /* request post parameters                                    */
  string  request_body;                   /* request post body                                          */
  string  certificate_path;               /* request cert path                                          */
  string  ca_path;                        /* request ca path                                            */
  string  key_path;                       /* request key path                                           */
  string  password;                       /* request password                                           */

  char*   read_cb_ptr;                    /* read callback ptr address (request_body ptr may change)    */
  int     verify_peer;                    /* ssl peer verification                                      */
  int     verify_host;                    /* ssl host verification                                      */
  int     debug;                          /* debug certain request                                      */
  long    timeout;                        /* request timeout                                            */
  long    expectations;                   /* header expectations for request continuation               */
} request;

typedef struct {
  request*     requests;                  /* request objects                                            */
  size_t       count;                     /* request count                                              */
} request_handler;

char logger_buffer[LOGGER_BUFFER_SIZE];   /* the buffer used for logger messages                        */

/* ============================================= FUNCTIONS ============================================= */

/* LIBCURL METHODS */
int request_pool(request_handler* request_handler);
struct curl_slist* define_request_headers(CURL *eh, request* request);
void init_curl_handle(CURLM *cm, int i, request* requests);
void setup_ssl_request(CURL *eh, request* request);
void setup_put_request(CURL *eh, request* request);
void setup_post_request(CURL *eh, request* request);

/* REQUEST HANDLER METHODS */
int init_requests(request_handler* handler);
int init_request_headers(request* request, int total_header_fields);

/* HELPERS METHODS */
void init_string(string *s);
size_t writefunc(void *ptr, size_t size, size_t nmemb, string *s);
void set_read_cb_ptr(request* request);
size_t read_callback(void *dest, size_t size, size_t nmemb, void *userp);
size_t memcpy_string(const char *str, string *s);
int is_https(const char* url);
int is_empty(const char* str);
int method_put(const char* method);
int method_get(const char* method);
int method_post(const char* method);

/* LUA API METHODS */
void free_request_handler(request_handler* handler);
request_handler* request_processor(lua_State* L);
void set_request_data(request* request, const char* key, const char* s_value);
void set_request_integers(request* request, const char* key, lua_Number number);
int set_request_headers(request* request, const char* key, lua_State* L);
void l_pushheaders(lua_State* L, char* response_headers_key, char* response_headers);
void l_pushtablestring(lua_State* L , char* key , char* value);
void l_pushtablenumber(lua_State* L, char* key, double value);
int generate_response(lua_State* L, request_handler* handler);

/* LOGGER METHODS */
void print_log(const char* severity, const char* func, const char* message);
void create_log_by_severity(unsigned char severity, const char* func, const char* message);
void create_log(unsigned char severity, const char* func, const char* message);
void log_debug_info(const char* func, const char* format, ...);
void log_info(const char* func, const char* format, ...);
void log_error(const char* func, const char* format, ...);
void log_fatal_error(const char* func, const char* format, ...);

/* ============================================= ENUMS ============================================= */

enum ERR {
  FDSET_ERROR = -1,
  MULTI_TIMEOUT = -2,
  INVALID_SELECT_VALUE = -3
};

enum LOG_LEVELS {
  L_FATAL_ERROR = 1,
  L_ERROR = 2,
  L_INFO = 3,
  L_DEBUG = 4
};

enum LOG_VERBOSE {
  VERBOSE_V = 1,
  VERBOSE_VV = 2,
  VERBOSE_VVV = 3
};

