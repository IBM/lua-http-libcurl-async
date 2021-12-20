#include "libcurl_async.h"

/**
 * :l_pushtablestring
 * Pushes key, value pairs to lua stack
 */
void l_pushtablestring(lua_State* L , char* key , char* value) {
    lua_pushstring(L, key);
    lua_pushstring(L, value);
    lua_settable(L, -3);
}

/**
 * :l_pushtablenumber
 * Pushes key, number pairs to lua stack
 */
void l_pushtablenumber(lua_State* L , char* key, double value) {
    lua_pushstring(L, key);
    lua_pushnumber(L, value);
    lua_settable(L, -3);
}

/**
 * :l_pushheaders
 * Pushes response headers table back to lua as (key, value)
 * where key is the header name, and value is the key header value
 */
void l_pushheaders(lua_State* L, char* response_headers_key, char* response_headers)
{
  size_t header_size, token_size, index_of, i;
  char* single_header, *e_token = NULL;
  char tbl_key[TBL_KEY_SZ], tbl_value[TBL_VAL_SZ];

  lua_pushstring(L, response_headers_key);
  lua_newtable(L);

  while ((single_header = strtok_r(response_headers, "\n\r", &response_headers))) {
    /* single_header looks like "content-type: text/html; charset=UTF-8"
        we'll ignore single_header's that doesn't comply with key, value structure */
    if (is_empty(single_header)) break;
    header_size = strlen(single_header);
    e_token = strchr(single_header, ':');
    if (!is_empty(e_token))
    {
      token_size = strlen(e_token);
      index_of = header_size - token_size;
      if (index_of >= TBL_KEY_SZ || token_size >= TBL_VAL_SZ-HEADER_SPACING) continue;
      for (i=0; i<index_of; i++) tbl_key[i] = tolower(single_header[i]);
      for (i=0; i<token_size-HEADER_SPACING; i++) tbl_value[i] = e_token[i+HEADER_SPACING];
      tbl_key[index_of] = '\0';
      tbl_value[token_size-HEADER_SPACING] = '\0';
      l_pushtablestring(L, tbl_key, tbl_value);
    } 
  }
  lua_settable(L, -3);
}

/**
 * :generate_response
 * Pushes request_handler response back to lua
 */
int generate_response(lua_State* L, request_handler* handler)
{
  size_t i;
  lua_newtable(L);  /* the main table to return    */
  
  for (i=0; i<handler->count; i++)
  {
    lua_pushstring(L, handler->requests[i].request_key.ptr);
    lua_newtable(L);  
    l_pushtablestring(L, "url",             handler->requests[i].url.ptr);
    l_pushtablenumber(L, "response_status", (double)handler->requests[i].response_status);
    l_pushtablestring(L, "response_body",   handler->requests[i].response_body.ptr);
    l_pushheaders(L,     "response_headers",handler->requests[i].response_headers.ptr);
    l_pushtablestring(L, "response_error",  handler->requests[i].response_err);
    lua_settable(L, -3);
  }
  return 1;
}

/**
 * :init_requests
 * Initiating each request initial data
 */
int init_requests(request_handler* handler)
{
  size_t total_requests = handler->count, i;
  handler->requests = (request*) malloc(sizeof(request) * total_requests);              /* requests allocation                 */
  if (handler->requests == NULL) return 0;

  for (i=0; i<total_requests; i++)
  {
    handler->requests[i].timeout              = DEFAULT_REQUEST_TIMEOUT;
    handler->requests[i].debug                =
    handler->requests[i].expectations         =
    handler->requests[i].verify_host          =
    handler->requests[i].header_fields.count  = 0;
    handler->requests[i].verify_peer          = 1;
    handler->requests[i].response_err[0]      = '\0';
    handler->requests[i].read_cb_ptr          = NULL;

    init_string(&handler->requests[i].request_key);
    init_string(&handler->requests[i].url);
    init_string(&handler->requests[i].response_body);
    init_string(&handler->requests[i].response_headers);
    init_string(&handler->requests[i].request_method);
    init_string(&handler->requests[i].post_params);
    init_string(&handler->requests[i].request_body);
    init_string(&handler->requests[i].certificate_path);
    init_string(&handler->requests[i].ca_path);
    init_string(&handler->requests[i].key_path);
    init_string(&handler->requests[i].password);
  }
  return 1;
}

/**
 * :set_request_integers
 * Sets a certain request integer values by the key to insert
 */
void set_request_integers(request* request, const char* key, lua_Number number)
{
  int i_value = (int)number;
  long l_value = (long)number;

  if (strcmp(key, "debug") == 0)
    request->debug = i_value;
  else if (strcmp(key, "expectations") == 0)
    request->expectations = (l_value > 0) ? l_value : DEFAULT_REQUEST_EXPECTATIONS;
  else if (strcmp(key, "verify_peer") == 0)
    request->verify_peer = i_value;
  else if (strcmp(key, "verify_host") == 0)
    request->verify_host = i_value;
  else if (strcmp(key, "timeout") == 0)
    request->timeout = (long)(((number > 0) ? number : DEFAULT_REQUEST_TIMEOUT) * MILLISECONDS);                /* 8 seconds timeout by default   */
}

/**
 * :set_request_data
 * Sets a certain request values by the key to insert
 * and the data type depends on that key.
 */
void set_request_data(request* request, const char* key, const char* s_value)
{
  if      (strcmp(key, "name") == 0) 
    memcpy_string(s_value, &request->request_key);
  else if (strcmp(key, "method") == 0) 
    memcpy_string(s_value, &request->request_method);
  else if (strcmp(key, "post_params") == 0) 
    memcpy_string(s_value, &request->post_params);
  else if (strcmp(key, "data") == 0) 
    memcpy_string(s_value, &request->request_body);
  else if (strcmp(key, "url") == 0) 
    memcpy_string(s_value, &request->url);
  else if (strcmp(key, "certificate") == 0) 
    memcpy_string(s_value, &request->certificate_path);
  else if (strcmp(key, "cafile") == 0) 
    memcpy_string(s_value, &request->ca_path);
  else if (strcmp(key, "key") == 0) 
    memcpy_string(s_value, &request->key_path);
  else if (strcmp(key, "password") == 0) 
    memcpy_string(s_value, &request->password);
}

/**
 * :init_request_headers
 * Initiating each request header fields
 */
int init_request_headers(request* request, int total_header_fields)
{
  size_t i;
  request->header_fields.count = total_header_fields;
  request->header_fields.headers = (string*) malloc(sizeof(string) * total_header_fields);
  if (request->header_fields.headers == NULL) return 0;
  for (i=0; i<total_header_fields; i++) init_string(&request->header_fields.headers[i]);
  return 1;
}

/**
 * :set_request_headers
 * Sets a certain request header fields
 */
int set_request_headers(request* request, const char* key, lua_State* L)
{
  size_t header_index = 0; /* headers count for each requests starts here */
  if (strcmp(key, "headers") != 0) return 1;
  if (!init_request_headers(request, (int)lua_objlen(L, -1))) return 0;
  switch (lua_type(L, -1))
  {
    case LUA_TTABLE:                                /* check for header tables only                */
      lua_pushnil(L);
      while(lua_next(L, -2) != 0) {
        if (lua_type(L, -1) == LUA_TTABLE) {
          lua_pushnil(L);
          while(lua_next(L, -2) != 0) {
            /* CASE INVALID HEADER */
            if (lua_type(L, -2) != LUA_TSTRING || lua_type(L, -1) != LUA_TSTRING) {
              lua_pop(L, 1);
              continue;
            } 
            memcpy_string(luaL_checkstring(L, -2), &request->header_fields.headers[header_index]);
            memcpy_string(": ", &request->header_fields.headers[header_index]);
            memcpy_string(luaL_checkstring(L, -1), &request->header_fields.headers[header_index]);
            lua_pop(L, 1);
          }
        }
        header_index++;
        lua_pop(L, 1);
      }
    break;
  }
  return 1;
}

/**
 * @request_handler
 * gets and initiates the request handler by lua params from lua to C
 **/
request_handler* request_processor(lua_State* L)
{
  size_t index = 0;
  const char *key = NULL;
  request_handler* handler = (request_handler*) malloc(sizeof(request_handler));
  if (handler == NULL) return NULL;

  if (lua_istable(L, 1)) {
    handler->count = lua_objlen(L, -1);                            /* sets the total requests */
    if (!init_requests(handler)) return NULL;
    
    lua_pushnil(L);
    while (lua_next(L, 1) != 0) {
      switch(lua_type(L, -1)) {
        case LUA_TTABLE:
          lua_pushnil(L);
          while(lua_next(L, -2) != 0) {                            
            if (lua_type(L, -2) == LUA_TSTRING)                    /* switching on keys       */
              key = luaL_checkstring(L, -2);
            
            switch (lua_type(L, -1)) {                             /* switching on values     */
              case LUA_TNUMBER:
              case LUA_TBOOLEAN:
                set_request_integers(&handler->requests[index], key, luaL_checknumber(L, -1));
              break;

              case LUA_TSTRING:
                set_request_data(&handler->requests[index], key, luaL_checkstring(L, -1));
              break;

              /* TREAT HEADERS IF SPECIFIED */
              case LUA_TTABLE:
                if (!set_request_headers(&handler->requests[index], key, L)) return NULL;              
              break;
            }
            lua_pop(L, 1);
          }
        break;
      }
      index++;
      lua_pop(L, 1);
    }
  }
  lua_settop(L, 0);  // clean stack, just to keep things lighter
  return handler;
}

/**
 * :free_request_handler
 * Simply freeing all request_handler object
 */
void free_request_handler(request_handler* handler)
{
  /* FREEING HANDLER OPTIONS */
  size_t i, header_index;

  /* FREEING HANDLER REQUESTS */
  for (i=0; i<handler->count; i++)
  {
    free(handler->requests[i].url.ptr);
    free(handler->requests[i].request_key.ptr);
    free(handler->requests[i].response_body.ptr);
    free(handler->requests[i].response_headers.ptr);
    
    free(handler->requests[i].request_method.ptr);
    free(handler->requests[i].post_params.ptr);

    if (handler->requests[i].read_cb_ptr != NULL)
      free(handler->requests[i].read_cb_ptr);

    free(handler->requests[i].certificate_path.ptr);
    free(handler->requests[i].ca_path.ptr);
    free(handler->requests[i].key_path.ptr);
    free(handler->requests[i].password.ptr);

    /* FREE HEADERS */
    if (handler->requests[i].header_fields.count > 0)
    {
      for (header_index=0; header_index<handler->requests[i].header_fields.count; header_index++)
        free(handler->requests[i].header_fields.headers[header_index].ptr);
      free(handler->requests[i].header_fields.headers);
    }
  }

  free(handler->requests);
  free(handler);
}
