//
//  ngx_stream_request_push_data_module.c
//  nginx-1.12
//
//  Created by xpwu on 2017/12/24.
//  Copyright © 2017年 xpwu. All rights reserved.
//

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_stream_request.h>

#include <ngx_stream_request_push_module.h>

#ifdef this_module
#undef this_module
#endif
#define this_module ngx_stream_request_push_data_module

#ifdef core_module
#undef core_module
#endif
#define core_module ngx_stream_request_core_module

typedef struct{
  ngx_uint_t sub_protocol;
  ngx_int_t handler_index;
} ngx_stream_request_push_data_svr_conf_t;

static void *ngx_stream_request_push_data_create_srv_conf(ngx_conf_t *cf);
static char *ngx_stream_request_push_data_merge_srv_conf(ngx_conf_t *cf
                                                    , void *parent, void *child);

static char *push_data_conf(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);

static ngx_command_t  ngx_stream_push_data_commands[] = {
  
  { ngx_string("push_data_subprotocol"),
    NGX_STREAM_MAIN_CONF|NGX_STREAM_SRV_CONF|NGX_CONF_NOARGS|NGX_CONF_TAKE1,
    push_data_conf,
    NGX_STREAM_SRV_CONF_OFFSET,
    0,
    NULL },
  
  ngx_null_command
};

static ngx_stream_module_t  ngx_stream_push_data_module_ctx = {
  NULL,
  NULL,            /* postconfiguration */
  
  NULL,  /* create main configuration */
  NULL,    /* init main configuration */
  
  ngx_stream_request_push_data_create_srv_conf,   /* create server configuration */
  ngx_stream_request_push_data_merge_srv_conf     /* merge server configuration */
};

ngx_module_t  ngx_stream_request_push_data_module = {
  NGX_MODULE_V1,
  &ngx_stream_push_data_module_ctx,           /* module context */
  ngx_stream_push_data_commands,              /* module directives */
  NGX_STREAM_MODULE,                     /* module type */
  NULL,                                  /* init master */
  NULL,                      /* init module */
  NULL,                     /* init process */
  NULL,                                  /* init thread */
  NULL,                                  /* exit thread */
  NULL,                     /* exit process */
  NULL,                                  /* exit master */
  NGX_MODULE_V1_PADDING
};

static void *ngx_stream_request_push_data_create_srv_conf(ngx_conf_t *cf) {
  ngx_stream_request_push_data_svr_conf_t* pscf;
  pscf = ngx_palloc(cf->pool, sizeof(ngx_stream_request_push_data_svr_conf_t));
  
  pscf->sub_protocol = NGX_CONF_UNSET_UINT;
  
  return pscf;
}

static char *ngx_stream_request_push_data_merge_srv_conf(ngx_conf_t *cf
                                                    , void *parent, void *child) {
  ngx_stream_request_push_data_svr_conf_t* conf = child;
  ngx_stream_request_push_data_svr_conf_t* prev = parent;
  
  ngx_conf_merge_uint_value(conf->sub_protocol
                            , prev->sub_protocol, NGX_CONF_UNSET_UINT);
  
  return NGX_CONF_OK;
}

static ngx_int_t handle_request(ngx_stream_request_t*);
static ngx_int_t build_response(ngx_stream_request_t*);

static char *push_data_conf(ngx_conf_t *cf, ngx_command_t *cmd, void *conf) {
 
  ngx_stream_request_push_data_svr_conf_t* pscf = conf;
  ngx_str_t* value = cf->args->elts;
  
  ngx_stream_request_handler_t* handler;
  
  handler = ngx_stream_request_add_handler(cf);
  pscf->handler_index = handler->index;
  handler->name = "push data";
  if (cf->args->nelts == 2) {
    handler->subprotocol_flag = ngx_atoi(value[1].data, value[1].len);
  } else {
    handler->subprotocol_flag = 0; // default subprotocol: 0
  }
  pscf->sub_protocol = handler->subprotocol_flag;
  handler->build_response = build_response;
  handler->handle_request = handle_request;
  
  return NGX_CONF_OK;
}

static ngx_int_t
push_data_dist_hander(ngx_stream_request_t*);

typedef struct{
  ngx_int_t done;
} request_src_ctx_t;

static ngx_int_t handle_request(ngx_stream_request_t* r) {
  request_src_ctx_t* ctx = ngx_stream_request_get_module_ctx(r, this_module);
  if (ctx == NULL) {
    ctx = ngx_pcalloc(r->pool, sizeof(request_src_ctx_t));
    ngx_stream_request_set_ctx(r, ctx, this_module);
  }
  if (ctx->done == 1) {
    return NGX_OK;
  }
  
  return ngx_stream_request_push_to_dist_process(r, push_data_dist_hander);
}

static ngx_int_t build_response(ngx_stream_request_t* r) {
  r->data->buf->last = r->data->buf->pos;
  return NGX_OK;
}

ngx_int_t
push_data_dist_hander(ngx_stream_request_t* r) {
  ngx_stream_handle_request_from(r, -1, 1);
  return NGX_OK;
}

