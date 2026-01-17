//@	{"target":{"name": "fd_activity_monitor.o"}}

#include "./fd_activity_monitor.hpp"

void prog::os_services::fd::activity_monitor::wait_for_and_distpatch_events()
{
	std::array<::epoll_event, 1024> events{};
	auto res = error_handling::do_while_eintr(
		::epoll_wait,
		m_epoll_fd.get().native_handle(),
		std::data(events),
		static_cast<int>(std::size(events)),
		0
	);
	if(res == -1)
	{ throw error_handling::system_error{"Failed to wait for events", errno}; }

	for(auto const& item : std::span{std::data(events), static_cast<size_t>(res)})
	{
		auto callback = static_cast<fd_event_callback*>(item.data.ptr);
		if((*callback)(epoll_event_to_activity_status(item.events)) == post_process_action::remove_entry_and_close_fd)
		{ delete callback; }
	}
}