#include <newt/list.h>
#include <newt/stomp.h>
#include <newt/common.h>
#include <newt/logger.h>

#include <assert.h>

#define QNAME_LENGTH (256)

struct attrinfo_t {
  char *qname;
  char *tid;
  char *receipt_id;
};

static int transaction_callback(frame_t *frame) {
  int ret = RET_ERROR;

  if(frame != NULL && frame->transaction_data != NULL) {
    enqueue((void *)frame, (char *)frame->transaction_data);

    ret = RET_SUCCESS;
  }

  return ret;
}

static int handler_destination(char *context, void *data) {
  struct attrinfo_t *attrinfo = (struct attrinfo_t *)data;
  int ret = RET_ERROR;

  if(attrinfo != NULL) {
    attrinfo->qname = context;

    ret = RET_SUCCESS;
  }

  return ret;
}

static int handler_transaction(char *context, void *data) {
  struct attrinfo_t *attrinfo = (struct attrinfo_t *)data;
  int ret = RET_ERROR;

  if(attrinfo != NULL) {
    attrinfo->tid = context;

    ret = RET_SUCCESS;
  }

  return ret;
}

static int handler_receipt(char *context, void *data) {
  struct attrinfo_t *attrinfo = (struct attrinfo_t *)data;
  int ret = RET_ERROR;

  if(attrinfo != NULL) {
    attrinfo->receipt_id = context;

    ret = RET_SUCCESS;
  }

  return ret;
}

static stomp_header_handler_t handlers[] = {
  {"destination:", handler_destination},
  {"transaction:", handler_transaction},
  {"receipt:", handler_receipt},
  {0},
};

frame_t *handler_stomp_send(frame_t *frame) {
  struct attrinfo_t attrinfo = {0};

  assert(frame != NULL);
  assert(frame->cinfo != NULL);

  if(iterate_header(&frame->h_attrs, handlers, &attrinfo) == RET_ERROR) {
    err("(handle_stomp_send) validation error");
    stomp_send_error(frame->sock, "failed to validate header\n");
    return NULL;
  }

  if(attrinfo.qname == NULL) {
    stomp_send_error(frame->sock, "no destination is specified\n");
    return NULL;
  }

  if(attrinfo.tid == NULL) {
    enqueue((void *)frame, attrinfo.qname);
  } else {
    frame->transaction_data = (void *)attrinfo.qname;
    transaction_add(attrinfo.tid, frame);
  }

  if(attrinfo.receipt_id != NULL) {
    stomp_send_receipt(frame->sock, attrinfo.receipt_id);
  }

  return frame;
}
