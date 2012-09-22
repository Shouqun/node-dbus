#include <v8.h>
#include <node.h>
#include <node_version.h>

#if !(NODE_MAJOR_VERSION == 0 && NODE_MINOR_VERSION < 8)

#include "context.h"

using namespace node;
using namespace v8;

static struct gcontext g_context;
static uv_prepare_t prepare_handle;
static uv_check_t check_handle;

GContext::GContext() {}

void GContext::Init()
{
	GMainContext *gc = g_main_context_default();

	struct gcontext *ctx = &g_context;

	if (!g_thread_supported())
		g_thread_init(NULL);

	g_main_context_acquire(gc);
	ctx->gc = g_main_context_ref(gc);
	ctx->fds = NULL;

	/* Prepare */
	uv_prepare_init(uv_default_loop(), &prepare_handle);
	uv_prepare_start(&prepare_handle, prepare_cb);

	/* Check */
	uv_check_init(uv_default_loop(), &check_handle);
	uv_check_start(&check_handle, check_cb);
}

void GContext::Uninit()
{
	struct gcontext *ctx = &g_context;

	uv_check_stop(&check_handle);
	uv_close((uv_handle_t *)&check_handle, NULL);

	uv_prepare_stop(&prepare_handle);
	uv_close((uv_handle_t *)&prepare_handle, NULL);

	g_main_context_unref(ctx->gc);
}

void GContext::poll_cb(uv_poll_t *handle, int status, int events)
{
	struct gcontext_pollfd *_pfd = (struct gcontext_pollfd *)handle->data;

	GPollFD *pfd = _pfd->pfd;

	pfd->revents |= pfd->events & ((events & UV_READABLE ? G_IO_IN : 0) | (events & UV_WRITABLE ? G_IO_OUT : 0));

	uv_poll_stop(handle);
	uv_close((uv_handle_t *)handle, NULL);
	delete _pfd;
}

void GContext::prepare_cb(uv_prepare_t *handle, int status)
{
	gint i;
	gint timeout;
	struct gcontext *ctx = &g_context;

	g_main_context_dispatch(ctx->gc);

	g_main_context_prepare(ctx->gc, &ctx->max_priority);

	/* Getting all sources from GLib main context */
	while(ctx->allocated_nfds < (ctx->nfds = g_main_context_query(ctx->gc,
			ctx->max_priority,
			&timeout,
			ctx->fds,
			ctx->allocated_nfds))) { 

		g_free(ctx->fds);
		g_free(ctx->poll_handles);

		ctx->allocated_nfds = 1;
		while (ctx->allocated_nfds < ctx->nfds) {
			ctx->allocated_nfds <<= 1;
		}

		ctx->fds = g_new(GPollFD, ctx->allocated_nfds);
		ctx->poll_handles = g_new(uv_poll_t, ctx->allocated_nfds);
	}

	/* Poll */
	for (i = 0; i < ctx->nfds; ++i) {
		GPollFD *pfd = ctx->fds + i;
		uv_poll_t *pt = ctx->poll_handles + i;

		struct gcontext_pollfd *data = new gcontext_pollfd;
		data->pfd = pfd;
		pt->data = data;

		pfd->revents = 0;

		uv_poll_init(uv_default_loop(), pt, pfd->fd);
		uv_poll_start(pt, UV_READABLE | UV_WRITABLE, poll_cb);
	}
}

void GContext::check_cb(uv_check_t *handle, int status)
{
	struct gcontext *ctx = &g_context;

	g_main_context_check(ctx->gc, ctx->max_priority, ctx->fds, ctx->nfds);
}

#endif
