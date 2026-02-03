//@	{"target":{"name": "epoll_instance.o"}}

#include "./epoll_instance.hpp"

void Pipe::os_services::io_multiplexer::epoll_instance::wait_for_and_distpatch_events()
{
	std::array<::epoll_event, 1024> events{};
	auto const res = error_handling::do_while_eintr(
		::epoll_wait,
		m_epoll_fd.get().native_handle(),
		std::data(events),
		static_cast<int>(std::size(events)),
		-1
	);
	if(res == -1)
	{ throw error_handling::system_error{"Failed to wait for events", errno}; }

	for(auto const& item : std::span{std::data(events), static_cast<size_t>(res)})
	{
		auto const data = static_cast<epoll_entry_data*>(item.data.ptr);
		if(
			epoll_fd_activity{
				*data,
				epoll_event_to_activity_status(item.events),
				m_epoll_fd.get()
			}.process().item_should_be_removed()
		)
		{
			m_listeners.erase(data->get_id());
		}
	}
}