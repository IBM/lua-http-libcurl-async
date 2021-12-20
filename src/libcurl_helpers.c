#include "libcurl_async.h"

/**
 * :is_empty
 * Simply returns if string is empty or not.
 */
int is_empty(const char* str)
{
  if (str == NULL) return 1;
  return (str[0] == '\0'); 
}

/**
 * :set_read_cb_ptr
 * request_body ptr address may change during the read callback, 
 * therefore, we keep a pointer to his address (read_cb_ptr). 
 * Later on, we use this address to free the object.
 */
void set_read_cb_ptr(request* request)
{
  if (request->request_body.ptr == NULL) return;
  request->read_cb_ptr = request->request_body.ptr;
}

/**
 * :method_put
 * Checks if method PUT is lower or uppercase
 */
int method_put(const char* method)
{
  if (is_empty(method)) return 0;
  return (strcmp(method, "PUT") == 0 || strcmp(method, "put") == 0);
}

/**
 * :method_get
 * Checks if method GET is lower or uppercase
 */
int method_get(const char* method)
{
  if (is_empty(method)) return 0;
  return (strcmp(method, "GET") == 0 || strcmp(method, "get") == 0);
}

/**
 * :method_post
 * Checks if method POST is lower or uppercase
 */
int method_post(const char* method)
{
  if (is_empty(method)) return 0;
  return (strcmp(method, "POST") == 0 || strcmp(method, "post") == 0);
}

/**
 * :is_https
 * Simply returns true if given url starts with 'https'.
 */
int is_https(const char* url)
{
  const char* https = {"https://"};
  size_t https_len = strlen(https), 
         url_len = strlen(url), i;

  /* no valid https url check   */
  if (url_len <= https_len) return 0;
  for (i=0; i<https_len; i++) 
    if (url[i] != https[i]) return 0;
  
  return 1;
}

/**
 * :init_string
 * Inits a string object of type 'string'
 */
void init_string(string *s) 
{
  s->len = 0;
  s->ptr = malloc(s->len+1);
  
  if (s->ptr == NULL) {
    log_fatal_error("init_string", "malloc() failed!");
    exit(EXIT_FAILURE);
  }
  s->ptr[0] = '\0';
}

/**
 * :writefunc
 * Writes a collection of chars to 'string' object. 
 * We use that function as libcurl callback function,
 * and also in 'memcpy_string' function just for copy
 * a certain string.
 */
size_t writefunc(void *ptr, size_t size, size_t nmemb, string *s)
{
  size_t new_len = s->len + size*nmemb;
  s->ptr = realloc(s->ptr, new_len+1);
  if (s->ptr == NULL) {
    log_fatal_error("writefunc", "realloc() failed!");
    exit(EXIT_FAILURE);
  }
  memcpy(s->ptr+s->len, ptr, size*nmemb);
  s->ptr[new_len] = '\0';
  s->len = new_len;
  return size*nmemb;
}

/**
 * :read_callback
 * The function gets a pointer to char (*userp) and feeds the destination (*dest) 
 * with that content. Be aware that userp address may change, so in order to free the 
 * address it points to, you should keep another pointer to this address before passing
 * it to read_callback. read more about post read callback func: 
 * https://curl.haxx.se/libcurl/c/post-callback.html
 */
size_t read_callback(void *dest, size_t size, size_t nmemb, void *userp)
{
  string *wt = (string*)userp;
  size_t buffer_size = size*nmemb;
  if(wt->len) {
    /* copy as much as possible from the source to the destination */ 
    size_t copy_this_much = wt->len;
    if(copy_this_much > buffer_size)
      copy_this_much = buffer_size;
    memcpy(dest, wt->ptr, copy_this_much);
 
    wt->ptr += copy_this_much;
    wt->len -= copy_this_much;
    return copy_this_much; /* we copied this many bytes */ 
  }
  return 0; /* no more data left to deliver */ 
}

/**
 * :memcpy_string
 * Adapter to writefunc.
 * Simply gets copies 'str' to 's' string. 
 */
size_t memcpy_string(const char *str, string *s)
{
  size_t string_length = strlen(str);
  return writefunc((void*)str, string_length, 1, s);
}
