#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "uv.h"
#include "uv_link_t.h"
#include "uv_http_t.h"

#define CHECK(V) if ((V) != 0) abort()
#define CHECK_ALLOC(V) if ((V) == NULL) abort()

typedef struct conn_s conn_t;
typedef struct req_s req_t;

static uv_tcp_t server;

struct conn_s {
  uv_tcp_t tcp;
  uv_link_source_t source;
  uv_link_observer_t observer;
  uv_http_t* http;
};

struct req_s {
  uv_http_req_t req;
  uv_link_observer_t observer;
  conn_t *conn;
};

static void close_cb(uv_link_t* link) {
  free(link->data);
}

static void read_cb(uv_link_observer_t* observer,
                    ssize_t nread,
                    const uv_buf_t* buf) {
  if (nread < 0) {
    uv_link_close((uv_link_t*) observer, close_cb);
    return;
  }
}


static void req_close_cb(uv_link_t* link) {
  req_t *r = link->data;
  if (!r->req.keep_alive)
  {
    uv_link_close((uv_link_t*)&r->conn->observer, close_cb);
  }
  free(link->data);
}


static void req_shutdown_cb(uv_link_t* link, int status, void* arg) {
  uv_link_close(link, req_close_cb);
}

static void req_body_write_cb(uv_link_t* link, int status, void* arg) {
  CHECK(uv_link_shutdown(link, req_shutdown_cb, arg));
}

static void req_head_write_cb(uv_link_t* link, int status, void* arg) {
  uv_buf_t buf;
  req_t *r = arg ? arg : link->data;
  buf = uv_buf_init("hello world", 11);
  CHECK(uv_link_write((uv_link_t*) &r->observer, &buf, 1, NULL,
                      req_body_write_cb, r));
}

static void req_active_cb(uv_http_req_t* req, int status) {
  req_t* r;
  uv_buf_t buf;
  uv_buf_t fields[4];
  uv_buf_t values[4];
  int i;

  r = req->data;

  buf = uv_buf_init("OK", 2);

  i=0;
  if(req->chunked==0) {
    fields[i] = uv_buf_init("Content-Length", 14);
    values[i] = uv_buf_init("11", 2);
    i++;
  }

  fields[i] = uv_buf_init("Connection", 10);
  if (req->keep_alive)
    values[i] = uv_buf_init("Keep-Alive", 10);
  else
    values[i] = uv_buf_init("Close", 5);
  i++;

  CHECK(uv_http_req_respond(req, 200, &buf, fields, values, i,
                            req_head_write_cb, r));

}

static void req_read_cb(uv_link_observer_t* observer, ssize_t nread,
                        const uv_buf_t* buf)
{
  req_t *r = observer->data;
  if (nread==UV_EOF)
  {
    uv_http_req_on_active(&r->req, req_active_cb);
  }
}

static void request_cb(uv_http_t* http, const char* url, size_t url_size, void* args) {
  req_t* r;

  CHECK_ALLOC(r = malloc(sizeof(*r)));
  CHECK(uv_http_accept(http, &r->req, r));
  CHECK(uv_link_observer_init(&r->observer));
  CHECK(uv_link_chain((uv_link_t*) &r->req, (uv_link_t*) &r->observer));
  CHECK(uv_link_read_start((uv_link_t*) &r->observer));

  r->observer.observer_read_cb = req_read_cb;
  r->observer.data = r;
  r->conn = args;
}


static void connection_cb(uv_stream_t* s, int status) {
  int err;
  conn_t* conn;

  CHECK_ALLOC(conn = malloc(sizeof(*conn)));

  CHECK(uv_tcp_init(uv_default_loop(), &conn->tcp));
  CHECK(uv_accept(s, (uv_stream_t*) &conn->tcp));
  CHECK(uv_link_source_init(&conn->source, (uv_stream_t*) &conn->tcp));
  CHECK(uv_link_observer_init(&conn->observer));

  CHECK_ALLOC(conn->http = uv_http_create(request_cb, &err, conn));
  CHECK(err);
  CHECK(uv_link_chain((uv_link_t*) &conn->source, (uv_link_t*) conn->http));
  CHECK(uv_link_chain((uv_link_t*) conn->http,
                      (uv_link_t*) &conn->observer));

  conn->observer.observer_read_cb = read_cb;
  conn->observer.data = conn;

  CHECK(uv_link_read_start((uv_link_t*) &conn->observer));
}


int main() {
  static const int kBacklog = 128;

  uv_loop_t* loop;
  struct sockaddr_in addr;

  loop = uv_default_loop();

  CHECK(uv_tcp_init(loop, &server));
  CHECK(uv_ip4_addr("0.0.0.0", 8080, &addr));
  CHECK(uv_tcp_bind(&server, (struct sockaddr*) &addr, 0));
  CHECK(uv_listen((uv_stream_t*) &server, kBacklog, connection_cb));

  fprintf(stderr, "Listening on 0.0.0.0:8080\n");

  CHECK(uv_run(loop, UV_RUN_DEFAULT));

  return 0;
}
