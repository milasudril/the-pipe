//@	{"target":{"name":"sync_server.test"}}

#include "./sync_server.hpp"
#include "src/os_services/ipc/unix_domain_socket.hpp"

#include <testfwk/testfwk.hpp>

TESTCASE(Pipe_worker_fwk_sync_server_init)
{
	Pipe::worker_fwk::sync_server server;
	EXPECT_EQ(std::size(server.socket_name()), Pipe::os_services::ipc::sunpath_maxlength);
}