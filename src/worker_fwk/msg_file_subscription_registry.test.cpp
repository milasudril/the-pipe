//@	{"target":{"name":"msg_file_subscription_registry.test"}}

#include "./msg_file_subscription_registry.hpp"

#include <testfwk/testfwk.hpp>

namespace
{
	struct my_port_collection
	{
		auto get_msg_file_output_ports()
		{
			return std::array{
				std::pair{
					Pipe::worker_fwk::port_id{0},
					Pipe::worker_fwk::msg_file_subscriber_barrier{
						Pipe::utils::bind_member_function<&my_port_collection::port_0_ready>(*this)
					}
				}
			};
		}

		void port_0_ready()
		{}

		Pipe::worker_fwk::port_id get_port_id(std::string const&)
		{
			return Pipe::worker_fwk::port_id{};
		}
	};
}

TESTCASE(Pipe_worker_fwk_msg_file_subscription_registry_construct_with_port_collection)
{
	my_port_collection port_collection{};
	Pipe::worker_fwk::msg_file_subscription_registry registry{port_collection};
}