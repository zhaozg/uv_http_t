#include "test-common.h"

static int req_closed;
static int write_cb_called;
static int shutdown_cb_called;

static uv_http_req_t req;

static void req_write_client(int fd) {

  client_send_str("GET /path HTTP/1.1\r\n\r\n");
  client_expect_str("HTTP/1.1 200 OK\r\n"
                    "ABC: DEF\r\n"
                    "X-Something: some-value\r\n"
                    "Transfer-Encoding: chunked\r\n\r\n"
                    "9\r\nsome body\r\n"
                    "0\r\n\r\n");
}


static void req_close_cb(uv_link_t* req) {
  req_closed++;
}


static int req_on_complete(uv_http_req_t* req) {
  uv_link_close((uv_link_t*) req, req_close_cb);
  return 0;
}


static void req_write_cb(uv_link_t* link, int status, void* arg) {
  CHECK_EQ(status, 0, "status should be zero");
  write_cb_called++;
}


static void req_shutdown_cb(uv_link_t* link, int status, void* arg) {
  CHECK_EQ(status, 0, "status should be zero");
  shutdown_cb_called++;
}


static void req_on_active(uv_http_req_t* req, int status) {
  uv_buf_t fields[2];
  uv_buf_t values[2];
  uv_buf_t msg;
  uv_buf_t body;

  CHECK_EQ(status, 0, "status should be zero");

  fields[0] = uv_buf_init("ABC", 3);
  values[0] = uv_buf_init("DEF", 3);
  fields[1] = uv_buf_init("X-Something", 11);
  values[1] = uv_buf_init("some-value", 10);

  msg = uv_buf_init("OK", 2);
  CHECK_EQ(uv_http_req_respond(req, 200, &msg, fields, values, 2, NULL, NULL),
           0,
           "uv_http_req_write");

  body = uv_buf_init("some body", 9);
  CHECK_EQ(uv_link_write((uv_link_t*) req, &body, 1, NULL, req_write_cb, NULL),
           0,
           "uv_link_write()");
  CHECK_EQ(uv_link_shutdown((uv_link_t*) req, req_shutdown_cb, NULL),
           0,
           "uv_link_shutdown()");

  CHECK_EQ(uv_link_read_stop((uv_link_t*) server.http), 0,
           "uv_read_stop(server)");
}

static void req_write_server(uv_http_t* http,
                               const char* url,
                               size_t url_len,
                               void* arg) {
  CHECK_EQ(uv_http_accept(http, &req), 0, "uv_http_accept()");

  uv_http_req_on_active(&req, req_on_active);

  req.on_headers_complete = req_on_complete;
}


TEST_IMPL(req_write) {
  http_client_server_test(req_write_client, req_write_server);
  CHECK_EQ(req_closed, 1, "req_closed == 1");
  CHECK_EQ(write_cb_called, 1, "write_cb_called == 1");
  CHECK_EQ(shutdown_cb_called, 1, "shutdown_cb_called == 1");
}
