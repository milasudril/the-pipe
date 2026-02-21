//@	{"target":{"name":"app.o"}}

#include "src/client/application.hpp"

namespace
{
	class pipe_application: public Pipe::client::application
	{
		public:
			virtual Pipe::client_ctl::client_application_info get_client_application_info() const override
			{
				return Pipe::client_ctl::client_application_info{};
			}
	};
}

std::unique_ptr<Pipe::client::application> Pipe::client::application::create()
{
	return std::unique_ptr<Pipe::client::application>{new pipe_application};
}

