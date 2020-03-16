#include "common.h"

static int uv_http_link_read_start(uv_link_t* link) {
  uv_http_t* http;
  http = (uv_http_t*) link;

  if (http->pending_err != 0)
    return http->pending_err;
  return uv_http_read_start(http, kUVHTTPSideConnection);
}


static int uv_http_link_read_stop(uv_link_t* link) {
  uv_http_t* http;
  http = (uv_http_t*) link;

  if (http->pending_err != 0)
    return http->pending_err;
  return uv_http_read_stop(http, kUVHTTPSideConnection);
}


static int uv_http_link_write(uv_link_t* link,
                              uv_link_t* source,
                              const uv_buf_t bufs[],
                              unsigned int nbufs,
                              uv_stream_t* send_handle,
                              uv_link_write_cb cb,
                              void* arg) {
  uv_http_t* http;
  http = (uv_http_t*) link;
  if (http->pending_err != 0)
    return http->pending_err;
  /* No support for writes */
  return UV_ENOSYS;
}


static int uv_http_link_try_write(uv_link_t* link,
                                  const uv_buf_t bufs[],
                                  unsigned int nbufs) {
  uv_http_t* http;
  http = (uv_http_t*) link;
  if (http->pending_err != 0)
    return http->pending_err;
  /* No support for writes */
  return UV_ENOSYS;
}


static int uv_http_link_shutdown(uv_link_t* link,
                                 uv_link_t* source,
                                 uv_link_shutdown_cb cb,
                                 void* arg) {
  uv_http_t* http;
  http = (uv_http_t*) link;
  if (http->pending_err != 0)
    return http->pending_err;
  return uv_link_propagate_shutdown(link->parent, source, cb, arg);
}


static void uv_http_link_close(uv_link_t* link, uv_link_t* source,
                               uv_link_close_cb cb) {
  uv_http_t* http;
  http = (uv_http_t*) link;

  uv_http_destroy(http, source, cb);
}


static void uv_http_link_alloc_cb_override(uv_link_t* link,
                                           size_t suggested_size,
                                           uv_buf_t* buf) {
  *buf = uv_buf_init(malloc(suggested_size), suggested_size);

  if (buf->base == NULL)
    buf->len = 0;
}


static void uv_http_link_read_cb_override(uv_link_t* link,
                                          ssize_t nread,
                                          const uv_buf_t* buf) {
  uv_http_t* http;
  int err;

  http = (uv_http_t*) link;

  if (nread >= 0)
    err = uv_http_consume(http, buf->base, nread);
  else
    err = nread;

  if (err != 0)
    uv_http_error(http, err);

  if (buf != NULL && buf->base)
    free(buf->base);
}


/* NOTE: Intentionally not static */
const char* uv_http_link_strerror(uv_link_t* link, int err) {
  switch (err) {
    case kUVHTTPErrDoubleShutdown:
      return "uv_http_t: double shutdown attempt";
    case kUVHTTPErrShutdownRequired:
      return "uv_http_t: can't close request without shutdown";
    case kUVHTTPErrDoubleRespond:
      return "uv_http_t: double respond attempt";
    case kUVHTTPErrResponseRequired:
      return "uv_http_t: response required before sending data";
    case kUVHTTPErrParserExecute:
      return "uv_http_t: http_parser_execute() error";
    case kUVHTTPErrConnectionReset:
      return "uv_http_t: connection reset";
    case kUVHTTPErrReqCallback:
      return "uv_http_t: request callback failure";
    case kUVHTTPErrWriteExceed:
      return "uv_http_t: response write bufs exceed limit";
    default:
      return NULL;
  }
}


uv_link_methods_t uv_http_methods = {
  .read_start = uv_http_link_read_start,
  .read_stop = uv_http_link_read_stop,

  .write = uv_http_link_write,
  .try_write = uv_http_link_try_write,
  .shutdown = uv_http_link_shutdown,
  .close = uv_http_link_close,
  .strerror = uv_http_link_strerror,

  .alloc_cb_override = uv_http_link_alloc_cb_override,
  .read_cb_override = uv_http_link_read_cb_override
};
