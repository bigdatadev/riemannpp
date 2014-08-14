#include "client.hpp"
#include "exception.hpp"

using namespace riemannpp;

client::client()
	: d_type(client_type::none)
{
	d_client = riemann_client_new();
}

client::client(client_type type, const std::string& host, int port)
	: d_type(client_type::none)
{
	d_client = riemann_client_create(riemann_client_type_t(type), host.c_str(), port);
}

client::~client() {
	if (d_client) {
		riemann_client_free(d_client);
		d_client = nullptr;
	}
}

void 
client::connect(client_type type, const std::string& host, int port) {
	int result = riemann_client_connect(d_client, riemann_client_type_t(type), host.c_str(), port);
	if (-1 == result) {
		throw new internal_exception();
	}
}

void 
client::disconnect() {
	int result = riemann_client_disconnect(d_client);
	if (-1 == result) {
		throw new internal_exception();
	}
}

void 
client::send_message(const message& m) {
	int result = riemann_client_send_message(d_client, (riemann_message_t*)m);
	if (-1 == result) {
		throw new internal_exception();
	}
}

void 
client::send_message_oneshot(const message& m) {
	int result = riemann_client_send_message_oneshot(d_client, (riemann_message_t*)m);
	if (-1 == result) {
		throw new internal_exception();
	}
}

//std::unique_ptr<message>
//client::recv() {
//	return std::unique_ptr<message>(new message()/*riemann_client_recv_message(d_client.get())*/);
//}