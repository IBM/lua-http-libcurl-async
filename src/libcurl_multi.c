#include "libcurl_async.h"

/**
 * :setup_ssl_request
 * Setup libcurl ssl dependencies
 */
void setup_ssl_request(CURL *eh, request* request)
{
  /* THE FLAG DEFINES MAXIMUM SUPPORTED TLS VERSION AS TLS V1.2 */
  if (LIBCURL_VERSION_NUM >= 0x073400)
    curl_easy_setopt(eh, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_2);
  else log_fatal_error("setup_ssl_request", 
                       "the current libcurl version isn't supporting TLS 1.2! (current: %s)", 
                       LIBCURL_VERSION);
  
  if (!is_empty(request->ca_path.ptr))
    curl_easy_setopt(eh, CURLOPT_CAINFO, request->ca_path.ptr);
  
  /* SETUP SSL CERTIFICATE */
  /* SETUP SSL PRIVATE KEY */
  /* SETUP SSL CERTIFICATE PASSWORD (IF THERES ONE) */
  if (!is_empty(request->certificate_path.ptr) && !is_empty(request->key_path.ptr))
  {
    curl_easy_setopt(eh, CURLOPT_SSLCERT, request->certificate_path.ptr);
    curl_easy_setopt(eh, CURLOPT_SSLCERTTYPE, PP_CERT_TYPE);
    curl_easy_setopt(eh, CURLOPT_SSLKEY, request->key_path.ptr);
    curl_easy_setopt(eh, CURLOPT_SSLKEYTYPE, PP_CERT_TYPE);

    if (!is_empty(request->password.ptr))
      curl_easy_setopt(eh, CURLOPT_KEYPASSWD, request->password.ptr);
  }

  /**
   * CURLOPT_SSL_VERIFYPEER:
   * When negotiating a TLS or SSL connection, the server sends a certificate indicating its identity. 
   * Curl verifies whether the certificate is authentic, i.e. that you can trust that the server is 
   * who the certificate says it is. This trust is based on a chain of digital signatures, rooted in 
   * certification authority (CA) certificates you supply. curl uses a default bundle of CA certificates 
   * (the path for that is determined at build time) and you can specify alternate certificates with the 
   * CURLOPT_CAINFO option or the CURLOPT_CAPATH option.
   * (default is 1)
   */
  if (request->verify_peer == 0)
    curl_easy_setopt(eh, CURLOPT_SSL_VERIFYPEER, 0L);

  /**
   * When CURLOPT_SSL_VERIFYHOST is 2, that certificate must indicate that the server 
   * is the server to which you meant to connect, or the connection fails. Simply put, 
   * it means it has to have the same name in the certificate as is in the URL you operate against.
   * (default is 2)
   */
  if (request->verify_host == 0)
    curl_easy_setopt(eh, CURLOPT_SSL_VERIFYHOST, 0L);
}

/**
 * :setup_put_request
 * Setup libcurl put dependencies
 */
void setup_put_request(CURL *eh, request* request)
{
  set_read_cb_ptr(request);
  curl_easy_setopt(eh, CURLOPT_READFUNCTION, read_callback);
  curl_easy_setopt(eh, CURLOPT_UPLOAD, 1L);
  curl_easy_setopt(eh, CURLOPT_READDATA, &request->request_body);
  curl_easy_setopt(eh, CURLOPT_INFILESIZE_LARGE, (curl_off_t)request->request_body.len);
}

/**
 * :setup_post_request
 * Setup libcurl post dependencies
 */
void setup_post_request(CURL *eh, request* request)
{
  set_read_cb_ptr(request);

  /* SETUP POST PARAMETERS */
  if (!is_empty(request->post_params.ptr))
    curl_easy_setopt(eh, CURLOPT_POSTFIELDS, request->post_params.ptr);
  
  /* SETUP POST BODY */
  else if (!is_empty(request->request_body.ptr))
  {
    curl_easy_setopt(eh, CURLOPT_POSTFIELDS, NULL);
    curl_easy_setopt(eh, CURLOPT_READFUNCTION, read_callback);
    curl_easy_setopt(eh, CURLOPT_READDATA, &request->request_body);
    curl_easy_setopt(eh, CURLOPT_POSTFIELDSIZE, (long)request->request_body.len);
  }
}

struct curl_slist* define_request_headers(CURL *eh, request* request)
{
  size_t header_index;
  struct curl_slist* libcurl_headers = NULL;

  for (header_index=0; header_index<request->header_fields.count; header_index++)
    libcurl_headers = curl_slist_append(libcurl_headers, request->header_fields.headers[header_index].ptr);
  
  if (!method_post(request->request_method.ptr) && !method_put(request->request_method.ptr))
    return libcurl_headers;  

  if (request->expectations == DEFAULT_REQUEST_EXPECTATIONS)
    libcurl_headers = curl_slist_append(libcurl_headers, DISABLE_EXPECT_100_CONTINUE);

  /* DEFINE WAIT IDLE (IN MILLISECONDS) IN CASE OF EXPECT-100 CONTINUE HEADERS */
  /* READ MORE: https://tools.ietf.org/html/rfc7231#section-5.1.1 */
  else
    curl_easy_setopt(eh, CURLOPT_EXPECT_100_TIMEOUT_MS, request->expectations);
  
  return libcurl_headers;
}

/**
 * :init_curl_handle
 * Initiates each handle with his own settings. 
 * After that, we're adding this handle to the multi handle
 * for farther processing.
 */
void init_curl_handle(CURLM *cm, int i, request* requests)
{
  CURL *eh = curl_easy_init();
  request* request = &requests[i];
  struct curl_slist* libcurl_headers = NULL;

  curl_easy_setopt(eh, CURLOPT_HEADER, 0L);
  curl_easy_setopt(eh, CURLOPT_URL, request->url.ptr);
  curl_easy_setopt(eh, CURLOPT_PRIVATE, request);
  
  if (request->debug)
    curl_easy_setopt(eh, CURLOPT_VERBOSE, 2L);
  
  /**
   * SSL CONFIGURATIONS
   */
  if (is_https(request->url.ptr))
    setup_ssl_request(eh, request);
    
  /* SETUP POST/GET/PUT METHODS */
  if (method_post(request->request_method.ptr))
    setup_post_request(eh, request);

  else if (method_get(request->request_method.ptr))
    curl_easy_setopt(eh, CURLOPT_HTTPGET, 1L);

  else if (method_put(request->request_method.ptr))
    setup_put_request(eh, request);
  
  /* SETUP REQUEST HEADERS */
  libcurl_headers = define_request_headers(eh, request);
  curl_easy_setopt(eh, CURLOPT_HTTPHEADER, libcurl_headers);
  
  /* KEEPING A POINTER TO SLIST, WE FREEING THAT LATER ON */
  request->header_fields.slist = libcurl_headers;

  /* TELLS LIBCURL TO FOLLOW REDIRECTION / MAXREDIRS: THE MAX REDIRECTIONS ALLOWED */
  curl_easy_setopt(eh, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(eh, CURLOPT_MAXREDIRS, 2L);

  /* DISABLE SIGNALS TO USE WITH THREADS */
  curl_easy_setopt(eh, CURLOPT_NOSIGNAL, 1L);

  /* FOR BODY RESPONSE */
  curl_easy_setopt(eh, CURLOPT_WRITEFUNCTION, writefunc);
  curl_easy_setopt(eh, CURLOPT_WRITEDATA, &request->response_body);

  /* FOR RESPONSE HEADERS */
  curl_easy_setopt(eh, CURLOPT_HEADERFUNCTION, writefunc);
  curl_easy_setopt(eh, CURLOPT_HEADERDATA, &request->response_headers);

  /* A SINGLE REQUEST TIMEOUT (8 SECONDS DEFAULT) */
  curl_easy_setopt(eh, CURLOPT_TIMEOUT_MS, request->timeout);
  
  /* PROVIDES A BUFFER TO STORE ERRORS IN */
  curl_easy_setopt(eh, CURLOPT_ERRORBUFFER, request->response_err);

  /* ADD NEW REQUEST HANDLE */
  curl_multi_add_handle(cm, eh);
}

/**
 * :request_pool
 * Handles the multi handler requests.
 * multi handler waits the whole requests to 
 * complete and updates request_handler->requests objects
 * which will pushed later back to lua.
 */
int request_pool(request_handler* request_handler)
{
  CURLM *multi_handler = NULL;
  CURLMsg *msg = NULL;
  long timeout;
  size_t i;
  int max_fds, queue_msgs, running_handles = -1;                    /* FDS or FD STANDS FOR FILE DESCRIPTOR */
  fd_set read_fd, write_fd, exc_fd;
  struct timeval timeout_object;

  /**
   * DEFINE MAX SIMULTANEOUSLY CONNECTIONS:
   */
  const int MAX_SIMULTANEOUSLY_CONNECTIONS = (request_handler->count > DEFAULT_MAX) ? 
                                                DEFAULT_MAX : request_handler->count;

  // curl_global_init(CURL_GLOBAL_ALL);
  multi_handler = curl_multi_init();

  /* WE CAN OPTIONALLY LIMIT THE TOTAL AMOUNT OF CONNECTIONS THIS MULTI HANDLE USES.
     Basically, that approach is similar to easy setop ('curl_easy_setopt'), but just for multi. */
  curl_multi_setopt(multi_handler, CURLMOPT_MAXCONNECTS, (long)MAX_SIMULTANEOUSLY_CONNECTIONS);

  for (i=0; i<MAX_SIMULTANEOUSLY_CONNECTIONS; ++i)
    init_curl_handle(multi_handler, i, request_handler->requests);

  while (running_handles) 
  {
    curl_multi_perform(multi_handler, &running_handles);

    if (running_handles) {
      FD_ZERO(&read_fd);
      FD_ZERO(&write_fd);
      FD_ZERO(&exc_fd);

      if (curl_multi_fdset(multi_handler, &read_fd, &write_fd, &exc_fd, &max_fds))
        return FDSET_ERROR;
      
      if (curl_multi_timeout(multi_handler, &timeout))
        return MULTI_TIMEOUT;

      if (timeout == -1) timeout = 100;
      if (max_fds == -1) sleep((unsigned int) timeout / 1000);
      else {
        timeout_object.tv_sec = timeout/1000;
        timeout_object.tv_usec = (timeout%1000)*1000;
        if (0 > select(max_fds + 1, &read_fd, &write_fd, &exc_fd, &timeout_object))
          return INVALID_SELECT_VALUE;
      }
    }

    /* READS FINISHED REQUESTS DATA HANDLES */
    while ((msg = curl_multi_info_read(multi_handler, &queue_msgs))) {
      if (msg->msg == CURLMSG_DONE) {
        request* current = NULL;
        CURL *e = msg->easy_handle;

        curl_easy_getinfo(msg->easy_handle, CURLINFO_PRIVATE, &current);

        /* SETS THE CURRENT REQUEST HANDLE RESPONSE STATUS */
        curl_easy_getinfo(msg->easy_handle, CURLINFO_RESPONSE_CODE, &current->response_status);
        
        curl_slist_free_all(current->header_fields.slist);    /* FREEING LIBCURL HEADERS LINKED LIST  */
        curl_multi_remove_handle(multi_handler, e);           /* REMOVING CURRENT LIBCURL EASY HANDLE */
        curl_easy_cleanup(e);
      }
      
      if (i < request_handler->count) {
        init_curl_handle(multi_handler, i++, request_handler->requests);
        running_handles++; /* just to prevent it from remaining at 0 if there are more requests to get    */
      }
    }
  }
  curl_multi_cleanup(multi_handler);
  // curl_global_cleanup();
  return 1;
}
