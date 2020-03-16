#include <string.h>
#include <assert.h>

#include "common.h"
#include "utils.h"

void uv_http_req_eof(uv_http_t* http, uv_http_req_t* req) {
  if (req->reading)
    return uv_link_propagate_read_cb((uv_link_t*) req, UV_EOF, NULL);

  req->pending_eof = 1;
}

int uv_http_req_consume(uv_http_t* http, uv_http_req_t* req,
                        const char* data, size_t size) {
  int err;

  while (req->reading && size != 0) {
    uv_buf_t buf;
    size_t avail;

    uv_link_propagate_alloc_cb((uv_link_t*) req, size, &buf);
    if (buf.len == 0) {
      uv_link_propagate_read_cb((uv_link_t*) req, UV_ENOBUFS, &buf);
      return 0;
    }

    avail = buf.len;
    if (avail > size)
      avail = size;

    memcpy(buf.base, data, avail);
    uv_link_propagate_read_cb((uv_link_t*) req, avail, &buf);

    size -= avail;
    data += avail;
  }

  /* Fully consumed */
  if (size == 0)
    return 0;

  /* Pending data */
  err = uv_http_data_queue(&http->pending.req_data, data, size);
  if (err != 0) {
    uv_http_req_error(http, req, err);
    return 0;
  }

  return 0;
}

void uv_http_req_on_active(uv_http_req_t* req, uv_http_req_active_cb cb) {
  if (req->http->active_req == req)
    return cb(req, 0);

  req->on_active = cb;
}

int uv_http_req_respond(uv_http_req_t* req,
                        unsigned short status,
                        const uv_buf_t* message,
                        const uv_buf_t header_fields[],
                        const uv_buf_t header_values[],
                        unsigned int header_count,
                        uv_link_write_cb cb,
                        void *arg) {
  uv_http_t* http;
  uv_buf_t buf_storage[ARRAY_LIMITED_SIZE];
  char status_str[16];
  size_t status_len;
  uv_buf_t* bufs;
  uv_buf_t* pbufs;
  unsigned int nbufs;
  unsigned int i;
  int err;
  int full;
  size_t total;

  http = req->http;

  /* Double respond */
  if (req->has_response)
    return kUVHTTPErrDoubleRespond;

  if (req->http->active_req != req)
    return UV_EAGAIN;

  /* TODO(indutny): default message */
  nbufs = header_count * 4 + 4;

  if (nbufs > ARRAY_SIZE(buf_storage))
    return kUVHTTPErrWriteExceed;

  bufs = buf_storage;

  status_len = snprintf(status_str, sizeof(status_str), "HTTP/%hu.%hu %hu ",
                        req->http_major, req->http_minor, status);

  bufs[0] = uv_buf_init(status_str, status_len);
  bufs[1] = *message;
  bufs[2] = uv_buf_init("\r\n", 2);
  for (i = 0; i < header_count; i++) {
    bufs[i * 4 + 3] = header_fields[i];
    bufs[i * 4 + 4] = uv_buf_init(": ", 2);
    bufs[i * 4 + 5] = header_values[i];
    bufs[i * 4 + 6] = uv_buf_init("\r\n", 2);
  }
  if (req->chunked)
    bufs[nbufs - 1] = uv_buf_init("Transfer-Encoding: chunked\r\n\r\n", 30);
  else
    bufs[nbufs - 1] = uv_buf_init("\r\n", 2);

  total = 0;
  for (i = 0; i < nbufs; i++)
    total += bufs[i].len;

  err = uv_link_try_write(http->parent, bufs, nbufs);
  if (err < 0)
    goto done;

  /* Fully written */
  full = 0;
  if ((size_t) err == total) {
    err = 0;
    full = 1;
    goto done;
  }

  /* Partial write, queue rest */
  pbufs = bufs;
  for (; nbufs > 0; pbufs++, nbufs--) {
    /* Slice */
    if (pbufs[0].len > (size_t) err) {
      pbufs[0].base += err;
      pbufs[0].len -= err;
      err = 0;
      break;

    /* Discard */
    } else {
      err -= pbufs[0].len;
    }
  }

  err = uv_link_propagate_write(http->parent, req->child, pbufs, nbufs,
                                NULL, cb, arg);

done:

  if (err == 0) {
    req->has_response = 1;
    if (full)
      cb(req->child, 0, arg);
  }

  return err;
}

int uv_http_req_prepare_write(uv_http_req_t* req,
                              char* prefix_storage, unsigned int prefix_size,
                              uv_buf_t* storage, unsigned int nstorage,
                              const uv_buf_t* bufs, unsigned int nbufs,
                              uv_buf_t** pbufs, unsigned int* npbufs) {
  size_t total;
  unsigned int i;
  int prefix_len;
  uv_buf_t* res;
  unsigned int nres;

  /* Response is required before writes */
  if (!req->has_response)
    return kUVHTTPErrResponseRequired;

  total = 0;
  for (i = 0; i < nbufs; i++)
    total += bufs[i].len;

  /* Zero chunked writes are used for shutdown */
  if (total == 0) {
    *pbufs = storage;
    *npbufs = 0;
    return 0;
  }

  if (!req->chunked) {
    *pbufs = (uv_buf_t*) bufs;
    *npbufs = nbufs;
    return 0;
  }

  nres = nbufs + 2;
  assert(nstorage>=nres);
  res = storage;

  prefix_len = snprintf(prefix_storage, prefix_size, "%llx\r\n",
                        (unsigned long long) total);
  res[0] = uv_buf_init(prefix_storage, prefix_len);
  for (i = 0; i < nbufs; i++)
    res[i + 1] = bufs[i];
  res[nres - 1] = uv_buf_init("\r\n", 2);

  *pbufs = res;
  *npbufs = nres;

  return 0;
}
