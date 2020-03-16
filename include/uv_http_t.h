#ifndef INCLUDE_UV_HTTP_H_
#define INCLUDE_UV_HTTP_H_

#include "uv.h"
#include "uv_link_t.h"
#ifdef USE_LLHTTP
#include "llhttp.h"
#else
#include "http_parser.h"
typedef http_parser llhttp_t;
typedef enum http_method llhttp_method_t;
#endif

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/* NOTE: can be cast to `uv_link_t` */
typedef struct uv_http_s uv_http_t;
typedef struct uv_http_req_s uv_http_req_t;

typedef void (*uv_http_req_handler_cb)(uv_http_t* http, const char* url,
                                       size_t url_size, void* arg);
typedef int (*uv_http_req_cb)(uv_http_req_t* req);
typedef int (*uv_http_req_value_cb)(uv_http_req_t* req, const char* value,
                                    size_t length);
typedef void (*uv_http_req_active_cb)(uv_http_req_t* req, int status);

/* Flags, actually */
enum uv_http_req_state_e {
  kUVHTTPReqStateEnd = 0x1,
  kUVHTTPReqStateFinished = 0x2,
};
typedef enum uv_http_req_state_e uv_http_req_state_t;

struct uv_http_req_s {
  UV_LINK_FIELDS

  /* Public fields */
  uv_http_t* http;

  unsigned short http_major;
  unsigned short http_minor;
  llhttp_method_t method;

  /* Use chunked encoding, `0` by default */
  unsigned int chunked:1;
  /* Keep connection alive, `0` by default */
  unsigned int keep_alive:1;

  uv_http_req_value_cb on_header_field;
  uv_http_req_value_cb on_header_value;
  uv_http_req_cb on_headers_complete;

  /* Invoked when the request becomes available for sending response and
   * doing writes
   * (Semi-private, use `uv_http_req_on_active()`)
   */
  uv_http_req_active_cb on_active;

  /* Private fields */
  unsigned int state: 2;
  unsigned int reading: 1;
  unsigned int pending_eof: 1;
  unsigned int has_response: 1;
  unsigned int shutdown: 1;
  int pending_err;
  uv_http_req_t* next;
};

UV_EXTERN uv_http_t* uv_http_create(uv_http_req_handler_cb cb, int* err, void* data);
UV_EXTERN int uv_http_accept(uv_http_t* http, uv_http_req_t* req, void* data);
UV_EXTERN void uv_http_error(uv_http_t* http, int err);

/* Request */

/* NOTE: `cb` may be executed synchronously */
UV_EXTERN void uv_http_req_on_active(uv_http_req_t* req,
                                     uv_http_req_active_cb cb);

UV_EXTERN int uv_http_req_respond(uv_http_req_t* req,
                                  unsigned short status,
                                  const uv_buf_t* message,
                                  const uv_buf_t header_fields[],
                                  const uv_buf_t header_values[],
                                  unsigned int header_count,
                                  uv_link_write_cb cb,
                                  void *data);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* INCLUDE_UV_LINK_H_ */
