#include <zyre.h>
#include <errno.h>
#include <mruby.h>
#include <mruby/array.h>
#include <mruby/class.h>
#include <mruby/data.h>
#include <mruby/error.h>
#include <mruby/string.h>
#include <mruby/throw.h>

static void
mrb_zyre_destroy(mrb_state* mrb, void* p)
{
    if (p) {
        zyre_stop((zyre_t*)p);
        zclock_sleep(100);
        zyre_destroy((zyre_t**)&p);
    }
}

static const struct mrb_data_type mrb_zyre_type = {
    "$i_mrb_zyre_type", mrb_zyre_destroy
};

static mrb_value
mrb_zyre_initialize(mrb_state* mrb, mrb_value self)
{
    errno = 0;
    char* name = NULL;

    mrb_get_args(mrb, "|z!", &name);

    zyre_t* zyre = zyre_new(name);
    if (zyre)
        mrb_data_init(self, zyre, &mrb_zyre_type);
    else
        mrb_sys_fail(mrb, "zyre_new");

    return self;
}

static mrb_value
mrb_zyre_print(mrb_state* mrb, mrb_value self)
{
    zyre_print((zyre_t*)DATA_PTR(self));

    return self;
}

static mrb_value
mrb_zyre_uuid(mrb_state* mrb, mrb_value self)
{
    const char* uuid = zyre_uuid((zyre_t*)DATA_PTR(self));

    return mrb_str_new_static(mrb, uuid, strlen(uuid));
}

static mrb_value
mrb_zyre_name(mrb_state* mrb, mrb_value self)
{
    const char* name = zyre_name((zyre_t*)DATA_PTR(self));

    return mrb_str_new_static(mrb, name, strlen(name));
}

static mrb_value
mrb_zyre_set_header(mrb_state* mrb, mrb_value self)
{
    char *name, *value;

    mrb_get_args(mrb, "zz", &name, &value);

    zyre_set_header((zyre_t*)DATA_PTR(self), name, "%s", value);

    return self;
}

static mrb_value
mrb_zyre_set_verbose(mrb_state* mrb, mrb_value self)
{
    zyre_set_verbose((zyre_t*)DATA_PTR(self));

    return self;
}

static mrb_value
mrb_zyre_set_port(mrb_state* mrb, mrb_value self)
{
    mrb_int port;

    mrb_get_args(mrb, "i", &port);

    zyre_set_port((zyre_t*)DATA_PTR(self), port);

    return self;
}

static mrb_value
mrb_zyre_set_interval(mrb_state* mrb, mrb_value self)
{
    mrb_int interval;

    mrb_get_args(mrb, "i", &interval);

    zyre_set_interval((zyre_t*)DATA_PTR(self), interval);

    return self;
}

static mrb_value
mrb_zyre_set_interface(mrb_state* mrb, mrb_value self)
{
    char* interface;

    mrb_get_args(mrb, "z", &interface);

    zyre_set_interface((zyre_t*)DATA_PTR(self), interface);

    return self;
}

static mrb_value
mrb_zyre_set_endpoint(mrb_state* mrb, mrb_value self)
{
    errno = 0;
    char* endpoint;

    mrb_get_args(mrb, "z", &endpoint);

    if (zyre_set_endpoint((zyre_t*)DATA_PTR(self), "%s", endpoint) == -1)
        mrb_sys_fail(mrb, "zyre_set_endpoint");

    return self;
}

static mrb_value
mrb_zyre_gossip_bind(mrb_state* mrb, mrb_value self)
{
    char* endpoint;

    mrb_get_args(mrb, "z", &endpoint);

    zyre_gossip_bind((zyre_t*)DATA_PTR(self), "%s", endpoint);

    return self;
}

static mrb_value
mrb_zyre_gossip_connect(mrb_state* mrb, mrb_value self)
{
    char* endpoint;

    mrb_get_args(mrb, "z", &endpoint);

    zyre_gossip_connect((zyre_t*)DATA_PTR(self), "%s", endpoint);

    return self;
}

static mrb_value
mrb_zyre_start(mrb_state* mrb, mrb_value self)
{
    errno = 0;

    if (zyre_start((zyre_t*)DATA_PTR(self)) == -1)
        mrb_sys_fail(mrb, "zyre_start");

    return self;
}

static mrb_value
mrb_zyre_stop(mrb_state* mrb, mrb_value self)
{
    zyre_stop((zyre_t*)DATA_PTR(self));

    return self;
}

static mrb_value
mrb_zyre_join(mrb_state* mrb, mrb_value self)
{
    char* group;

    mrb_get_args(mrb, "z", &group);

    zyre_join((zyre_t*)DATA_PTR(self), group);

    return self;
}

static mrb_value
mrb_zyre_leave(mrb_state* mrb, mrb_value self)
{
    char* group;

    mrb_get_args(mrb, "z", &group);

    zyre_leave((zyre_t*)DATA_PTR(self), group);

    return self;
}

static mrb_value
mrb_zyre_recv(mrb_state* mrb, mrb_value self)
{
    errno = 0;
    mrb_value result = mrb_nil_value();
    zframe_t* zframe = NULL;

    zmsg_t* zmsg = zyre_recv((zyre_t*)DATA_PTR(self));
    if (zmsg) {
        struct mrb_jmpbuf* prev_jmp = mrb->jmp;
        struct mrb_jmpbuf c_jmp;

        MRB_TRY(&c_jmp)
        {
            mrb->jmp = &c_jmp;
            result = mrb_ary_new_capa(mrb, zmsg_size(zmsg));
            int ai = mrb_gc_arena_save(mrb);
            zframe = zmsg_pop(zmsg);
            while (zframe) {
                mrb_value s = mrb_str_new(mrb, zframe_data(zframe), zframe_size(zframe));
                zframe_destroy(&zframe);
                mrb_ary_push(mrb, result, s);
                mrb_gc_arena_restore(mrb, ai);
                zframe = zmsg_pop(zmsg);
            }
            zmsg_destroy(&zmsg);
            mrb->jmp = prev_jmp;
        }
        MRB_CATCH(&c_jmp)
        {
            mrb->jmp = prev_jmp;
            zframe_destroy(&zframe);
            zmsg_destroy(&zmsg);
            MRB_THROW(mrb->jmp);
        }
        MRB_END_EXC(&c_jmp);
    }
    else
        mrb_sys_fail(mrb, "zyre_recv");

    return result;
}

static mrb_value
mrb_zyre_whisper(mrb_state* mrb, mrb_value self)
{
    char* peer;
    char* msg_str;
    mrb_int msg_len;
    zmsg_t* msg;

    mrb_get_args(mrb, "zs", &peer, &msg_str, &msg_len);

    errno = 0;

    msg = zmsg_new();
    if (msg) {
        if (zmsg_addmem(msg, msg_str, msg_len) == -1) {
            zmsg_destroy(&msg);
            mrb_sys_fail(mrb, "zmsg_addmem");
        }
        zyre_whisper((zyre_t*)DATA_PTR(self), peer, &msg);
    }
    else
        mrb_sys_fail(mrb, "zmsg_new");

    return self;
}

static mrb_value
mrb_zyre_shout(mrb_state* mrb, mrb_value self)
{
    char* group;
    char* msg_str;
    mrb_int msg_len;
    zmsg_t* msg;

    mrb_get_args(mrb, "zs", &group, &msg_str, &msg_len);

    errno = 0;

    msg = zmsg_new();
    if (msg) {
        if (zmsg_addmem(msg, msg_str, msg_len) == -1) {
            zmsg_destroy(&msg);
            mrb_sys_fail(mrb, "zmsg_addmem");
        }
        zyre_shout((zyre_t*)DATA_PTR(self), group, &msg);
        mrb->jmp = prev_jmp;
    }
    else
        mrb_sys_fail(mrb, "zmsg_new");

    return self;
}

static mrb_value
mrb_zyre_peers(mrb_state* mrb, mrb_value self)
{
    zlist_t* peers = zyre_peers((zyre_t*)DATA_PTR(self));

    struct mrb_jmpbuf* prev_jmp = mrb->jmp;
    struct mrb_jmpbuf c_jmp;
    mrb_value result = mrb_nil_value();

    MRB_TRY(&c_jmp)
    {
        mrb->jmp = &c_jmp;
        int ai = mrb_gc_arena_save(mrb);
        result = mrb_ary_new_capa(mrb, zlist_size(peers));
        char* peer = (char*)zlist_pop(peers);
        while (peer) {
            mrb_value s = mrb_str_new_cstr(mrb, peer);
            zstr_free(&peer);
            mrb_ary_push(mrb, result, s);
            mrb_gc_arena_restore(mrb, ai);
            peer = (char*)zlist_pop(peers);
        }

        zlist_destroy(&peers);
        mrb->jmp = prev_jmp;
    }
    MRB_CATCH(&c_jmp)
    {
        mrb->jmp = prev_jmp;
        zlist_destroy(&peers);
        MRB_THROW(mrb->jmp);
    }
    MRB_END_EXC(&c_jmp);

    return result;
}

static mrb_value
mrb_zyre_own_groups(mrb_state* mrb, mrb_value self)
{
    zlist_t* own_groups = zyre_own_groups((zyre_t*)DATA_PTR(self));

    struct mrb_jmpbuf* prev_jmp = mrb->jmp;
    struct mrb_jmpbuf c_jmp;
    mrb_value result = mrb_nil_value();

    MRB_TRY(&c_jmp)
    {
        mrb->jmp = &c_jmp;
        int ai = mrb_gc_arena_save(mrb);
        result = mrb_ary_new_capa(mrb, zlist_size(own_groups));
        char* own_group = (char*)zlist_pop(own_groups);
        while (own_group) {
            mrb_value s = mrb_str_new_cstr(mrb, own_group);
            zstr_free(&own_group);
            mrb_ary_push(mrb, result, s);
            mrb_gc_arena_restore(mrb, ai);
            own_group = (char*)zlist_pop(own_groups);
        }

        zlist_destroy(&own_groups);
        mrb->jmp = prev_jmp;
    }
    MRB_CATCH(&c_jmp)
    {
        mrb->jmp = prev_jmp;
        zlist_destroy(&own_groups);
        MRB_THROW(mrb->jmp);
    }
    MRB_END_EXC(&c_jmp);

    return result;
}

static mrb_value
mrb_zyre_peer_groups(mrb_state* mrb, mrb_value self)
{
    zlist_t* peer_groups = zyre_peer_groups((zyre_t*)DATA_PTR(self));

    struct mrb_jmpbuf* prev_jmp = mrb->jmp;
    struct mrb_jmpbuf c_jmp;
    mrb_value result = mrb_nil_value();

    MRB_TRY(&c_jmp)
    {
        mrb->jmp = &c_jmp;
        int ai = mrb_gc_arena_save(mrb);
        result = mrb_ary_new_capa(mrb, zlist_size(peer_groups));
        char* peer_group = (char*)zlist_pop(peer_groups);
        while (peer_group) {
            mrb_value s = mrb_str_new_cstr(mrb, peer_group);
            zstr_free(&peer_group);
            mrb_ary_push(mrb, result, s);
            mrb_gc_arena_restore(mrb, ai);
            peer_group = (char*)zlist_pop(peer_groups);
        }

        zlist_destroy(&peer_groups);
        mrb->jmp = prev_jmp;
    }
    MRB_CATCH(&c_jmp)
    {
        mrb->jmp = prev_jmp;
        zlist_destroy(&peer_groups);
        MRB_THROW(mrb->jmp);
    }
    MRB_END_EXC(&c_jmp);

    return result;
}

static mrb_value
mrb_zyre_peer_address(mrb_state* mrb, mrb_value self)
{
    errno = 0;
    char* peer;

    mrb_get_args(mrb, "z", &peer);

    char* address = zyre_peer_address((zyre_t*)DATA_PTR(self), peer);
    if (address) {
        mrb_value s = mrb_str_new_cstr(mrb, address);
        zstr_free(&address);
        return s;
    }
    else
        mrb_sys_fail(mrb, "zyre_peer_address");

    return self;
}

static mrb_value
mrb_zyre_peer_header_value(mrb_state* mrb, mrb_value self)
{
    errno = 0;
    char *peer, *name;

    mrb_get_args(mrb, "zz", &peer, &name);

    char* value = zyre_peer_header_value((zyre_t*)DATA_PTR(self), peer, name);
    if (value) {
        mrb_value s = mrb_str_new_cstr(mrb, value);
        zstr_free(&value);
        return s;
    }
    else if (errno != 0)
        mrb_sys_fail(mrb, "zyre_peer_header_value");
    else
        return mrb_nil_value();

    return self;
}

static mrb_value
mrb_zyre_socket(mrb_state* mrb, mrb_value self)
{
    zsock_t* zsock = zyre_socket((zyre_t*)DATA_PTR(self));
    struct RClass* czmq_mod = mrb_module_get(mrb, "CZMQ");
    struct RClass* zsock_cl = mrb_class_get_under(mrb, czmq_mod, "Zsock");
    mrb_value czmq_zsock = mrb_obj_value(zsock_cl);
    mrb_value zsock_value = mrb_cptr_value(mrb, zsock);

    return mrb_funcall(mrb, czmq_zsock, "new_from", 1, zsock_value);
}

void mrb_mruby_zyre_gem_init(mrb_state* mrb)
{
    struct RClass* zyre_class;

    zyre_class = mrb_define_class(mrb, "Zyre", mrb->object_class);
    MRB_SET_INSTANCE_TT(zyre_class, MRB_TT_DATA);
    mrb_define_method(mrb, zyre_class, "initialize", mrb_zyre_initialize, MRB_ARGS_OPT(1));
    mrb_define_method(mrb, zyre_class, "print", mrb_zyre_print, MRB_ARGS_NONE());
    mrb_define_method(mrb, zyre_class, "uuid", mrb_zyre_uuid, MRB_ARGS_NONE());
    mrb_define_method(mrb, zyre_class, "name", mrb_zyre_name, MRB_ARGS_NONE());
    mrb_define_method(mrb, zyre_class, "[]=", mrb_zyre_set_header, MRB_ARGS_REQ(2));
    mrb_define_method(mrb, zyre_class, "set_verbose", mrb_zyre_set_verbose, MRB_ARGS_NONE());
    mrb_define_method(mrb, zyre_class, "port=", mrb_zyre_set_port, MRB_ARGS_REQ(1));
    mrb_define_method(mrb, zyre_class, "interval=", mrb_zyre_set_interval, MRB_ARGS_REQ(1));
    mrb_define_method(mrb, zyre_class, "interface=", mrb_zyre_set_interface, MRB_ARGS_REQ(1));
    mrb_define_method(mrb, zyre_class, "endpoint=", mrb_zyre_set_endpoint, MRB_ARGS_REQ(1));
    mrb_define_method(mrb, zyre_class, "gossip_bind", mrb_zyre_gossip_bind, MRB_ARGS_REQ(1));
    mrb_define_method(mrb, zyre_class, "gossip_connect", mrb_zyre_gossip_connect, MRB_ARGS_REQ(1));
    mrb_define_method(mrb, zyre_class, "start", mrb_zyre_start, MRB_ARGS_NONE());
    mrb_define_method(mrb, zyre_class, "stop", mrb_zyre_stop, MRB_ARGS_NONE());
    mrb_define_method(mrb, zyre_class, "join", mrb_zyre_join, MRB_ARGS_REQ(1));
    mrb_define_method(mrb, zyre_class, "leave", mrb_zyre_leave, MRB_ARGS_REQ(1));
    mrb_define_method(mrb, zyre_class, "recv", mrb_zyre_recv, MRB_ARGS_NONE());
    mrb_define_method(mrb, zyre_class, "whisper", mrb_zyre_whisper, MRB_ARGS_REQ(1) | MRB_ARGS_REST());
    mrb_define_method(mrb, zyre_class, "shout", mrb_zyre_shout, MRB_ARGS_REQ(1) | MRB_ARGS_REST());
    mrb_define_method(mrb, zyre_class, "peers", mrb_zyre_peers, MRB_ARGS_NONE());
    mrb_define_method(mrb, zyre_class, "own_groups", mrb_zyre_own_groups, MRB_ARGS_NONE());
    mrb_define_method(mrb, zyre_class, "peer_groups", mrb_zyre_peer_groups, MRB_ARGS_REQ(1));
    mrb_define_method(mrb, zyre_class, "peer_address", mrb_zyre_peer_address, MRB_ARGS_REQ(1));
    mrb_define_method(mrb, zyre_class, "peer_header_value", mrb_zyre_peer_header_value, MRB_ARGS_REQ(2));
    mrb_define_method(mrb, zyre_class, "socket", mrb_zyre_socket, MRB_ARGS_NONE());
}

void mrb_mruby_zyre_gem_final(mrb_state* mrb)
{
}
