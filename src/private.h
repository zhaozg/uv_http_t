#ifndef SRC_PRIVATE_H_
#define SRC_PRIVATE_H_

#ifdef USE_LLHTTP
#include "llhttp.h"
#else
#include "http_parser.h"
typedef http_parser llhttp__t;
typedef http_parser_settings llhttp_settings_t;
#endif
#include "uv_link_t.h"

#include "utils.h"
#include "queue.h"

#define ARRAY_LIMITED_SIZE 512

enum uv_http_header_state_e {
  kUVHTTPHeaderStateURL,
  kUVHTTPHeaderStateField,
  kUVHTTPHeaderStateValue,
  kUVHTTPHeaderStateComplete
};
typedef enum uv_http_header_state_e uv_http_header_state_t;

struct uv_http_s {
  UV_LINK_FIELDS

  uv_http_req_handler_cb request_handler;

  unsigned int pending_accept: 1;
  uv_http_req_t* active_req;
  uv_http_req_t* last_req;
  unsigned int active_reqs;

  int pending_err;

  uv_link_close_cb close_cb;
  uv_link_t* close_source;

  unsigned int reading:2;

  llhttp_t parser;
#ifndef USE_LLHTTP
  llhttp_settings_t *parser_settings;
#endif

  struct {
    uv_http_data_t data;
    uv_http_data_t req_data;

    uv_http_data_t url_or_header;
    uv_http_header_state_t header_state;

    /* TODO(indutny): consider increasing it, the idea is to handle the
     * most common length of URLs */
    char storage[2048];
  } pending;
};

/* NOTE: used as flags too */
enum uv_http_side_e {
  kUVHTTPSideRequest = 1,
  kUVHTTPSideConnection = 2
};
typedef enum uv_http_side_e uv_http_side_t;

enum uv_http_error_e {
  kUVHTTPErrDoubleShutdown       = UV_ERRNO_MAX - 1,
  kUVHTTPErrShutdownRequired     = UV_ERRNO_MAX - 2,
  kUVHTTPErrDoubleRespond        = UV_ERRNO_MAX - 3,
  kUVHTTPErrResponseRequired     = UV_ERRNO_MAX - 4,
  kUVHTTPErrParserExecute        = UV_ERRNO_MAX - 5,
  kUVHTTPErrConnectionReset      = UV_ERRNO_MAX - 6,
  kUVHTTPErrReqCallback          = UV_ERRNO_MAX - 7,
  kUVHTTPErrWriteExceed          = UV_ERRNO_MAX - 8,
};

static const unsigned int kReadingMask = kUVHTTPSideRequest |
                                         kUVHTTPSideConnection;

extern uv_link_methods_t uv_http_methods;
extern uv_link_methods_t uv_http_req_methods;

void uv_http_destroy(uv_http_t* http, uv_link_t* source, uv_link_close_cb cb);

int uv_http_consume(uv_http_t* http, const char* data, size_t size);
void uv_http_error(uv_http_t* http, int err);
void uv_http_on_req_finish(uv_http_t* http, uv_http_req_t* req);

int uv_http_read_start(uv_http_t* http, uv_http_side_t side);
int uv_http_read_stop(uv_http_t* http, uv_http_side_t side);

void uv_http_maybe_close(uv_http_t* http);

const char* uv_http_link_strerror(uv_link_t* link, int err);

/* Request related */
void uv_http_close_req(uv_http_t* http, uv_http_req_t* req);
void uv_http_req_error(uv_http_t* http, uv_http_req_t* req, int err);
void uv_http_req_eof(uv_http_t* http, uv_http_req_t* req);
int uv_http_req_consume(uv_http_t* http, uv_http_req_t* req,
                        const char* data, size_t size);
int uv_http_req_prepare_write(uv_http_req_t* req,
                              char* prefix_storage, unsigned int prefix_size,
                              uv_buf_t* storage, unsigned int nstorage,
                              const uv_buf_t* bufs, unsigned int nbufs,
                              uv_buf_t** pbufs, unsigned int* npbufs);

#endif  /* SRC_PRIVATE_H_ */
