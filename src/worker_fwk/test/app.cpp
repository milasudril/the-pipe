//@	{"target":{"name":"app.o"}}

#include "src/worker_fwk/application.hpp"
#include "src/worker_ctl/data_stream_info.hpp"
#include "src/worker_ctl/worker_application_info.hpp"

namespace
{
	class pipe_application: public Pipe::worker_fwk::application
	{
		public:
			virtual Pipe::worker_ctl::worker_application_info get_worker_application_info() const override
			{
				Pipe::worker_ctl::input_port_info_map inputs;
				std::vector<Pipe::worker_ctl::port_capability> input_accepts;
				input_accepts.push_back(
					Pipe::worker_ctl::port_capability{
						.format = Pipe::worker_ctl::data_format_info{
							.type = Pipe::worker_ctl::type_descriptor{
								.ns_path = std::vector<std::string>{"builtins", "data_format_schemata"},
								.name = "mime",
							},
							.value = jopp::value{"text/plain"}
						},
						.transport_method = Pipe::worker_ctl::type_descriptor{
							.ns_path = std::vector<std::string>{"builtins", "transport_methods"},
							.name = "message_file"
						}
					}
				);
				inputs.insert(
					std::pair{
						"input",
						Pipe::worker_ctl::input_port_info{
							.accepts = std::move(input_accepts)
						}
					}
				);
				Pipe::worker_ctl::output_port_info_map outputs;
				outputs.insert(
					std::pair{
						"result",
						Pipe::worker_ctl::port_capability{
							.format = Pipe::worker_ctl::data_format_info{
								.type = Pipe::worker_ctl::type_descriptor{
									.ns_path = std::vector<std::string>{"builtins", "data_format_schemata"},
									.name = "mime"
								},
								.value = jopp::value{"text/plain"}
							},
							.transport_method = Pipe::worker_ctl::type_descriptor{
								.ns_path = std::vector<std::string>{"builtins", "transport_methods"},
								.name = "message_file"
							}
						}
					}
				);
				return Pipe::worker_ctl::worker_application_info{
					.display_name = "Test application",
					.inputs = std::move(inputs),
					.outputs = std::move(outputs)
				};
			}
	};
}

std::unique_ptr<Pipe::worker_fwk::application> Pipe::worker_fwk::application::create()
{
	return std::unique_ptr<Pipe::worker_fwk::application>{new pipe_application};
}

