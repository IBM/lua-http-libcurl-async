#include "libcurl_async.h"

/* Flag for the first time lib usage */
static char first_time_library_used = 1;

/**
 * :error
 * Throws an error back to lua,
 * should be treated in pcall
 */
static int error(lua_State *L, const char* error_message)
{
  lua_pushstring(L, error_message);
	return lua_error(L);
}

/**
 * :handle_request
 * The entry point for each lua async request.
 */
static int handle_request(lua_State* L) {
  int returned_status, returned_objects = 0;
  request_handler* handler = request_processor(L);
  
  /* CASE init_requests ALLOCATION FAILED */
  if (handler == NULL) return error(L, "requests allocation failed");
  
  /* case first time library run */
  if (first_time_library_used)
  {
    curl_global_init(CURL_GLOBAL_ALL);
    first_time_library_used = 0;
  }

  returned_status = request_pool(handler);
  switch (returned_status)
  {
    case FDSET_ERROR:
      free_request_handler(handler);
      return error(L, "error in file descriptors set operation");
    case MULTI_TIMEOUT:
      free_request_handler(handler);
      return error(L, "multi interface timeout");
    case INVALID_SELECT_VALUE:
      free_request_handler(handler);
      return error(L, "file descriptors select result is invalid");
    default:
    break;
  }  
  returned_objects = generate_response(L, handler);
  free_request_handler(handler);
  return returned_objects;
}

/**
 * @struct luaL_Reg
 * an internal mapping for the lua stack stracture.
 * here we can extend the internal stack functions.
 **/
static const struct luaL_Reg lib_mapping [] = 
{
  {"request", handle_request},
  {NULL, NULL}
};

/**
 * :luaopen_lua_async_http
 * An internal lua registeration for lua_async_http library.
 **/
int luaopen_lua_async_http(lua_State* L) {
    luaL_register(L, "lua_async_http", lib_mapping);
    return 1;
}
