
/* types of requests */

#ifndef HTTP_REQ_H
#define HTTP_REQ_H

#include <stdio.h>
#include "http_hdrs.h"
#include "http_trans.h"

typedef enum http_req_type {
  http_req_type_get = 0,
  http_req_type_options,
  http_req_type_head,
  http_req_type_post,
  http_req_type_put,
  http_req_type_delete,
  http_req_type_trace,
  http_req_type_connect,
  http_req_type_propfind,
  http_req_type_proppatch,
  http_req_type_mkcol,
  http_req_type_copy,
  http_req_type_move,
  http_req_type_lock,
  http_req_type_unlock
} http_req_type;

typedef enum http_req_state_tag {
  http_req_state_start = 0,
  http_req_state_sending_request,
  http_req_state_sending_headers,
  http_req_state_sending_body
} http_req_state;

/* same character representations as above. */

extern const char *http_req_type_char[];

typedef struct http_req_tag {
  http_req_type type;
  float          http_ver;
  char          *host;
  char          *full_uri;
  char          *resource;
  char          *body;
  int            body_len;
  http_hdr_list *headers;
  http_req_state state;
} http_req;

http_req *
http_req_new(void);

void
http_req_destroy(http_req *a_req);

int
http_req_prepare(http_req *a_req);

int
http_req_send(http_req *a_req, http_trans_conn *a_conn);

#endif /* HTTP_REQ_H */
