//@	{"target":{"name":"app.o"}}

#include "src/worker/application.hpp"

namespace
{
	class pipe_application: public Pipe::worker::application
	{
		public:
			virtual Pipe::worker_ctl::worker_application_info get_worker_application_info() const override
			{
				return Pipe::worker_ctl::worker_application_info{};
			}
	};
}

std::unique_ptr<Pipe::worker::application> Pipe::worker::application::create()
{
	return std::unique_ptr<Pipe::worker::application>{new pipe_application};
}

