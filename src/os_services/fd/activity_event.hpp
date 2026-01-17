#ifndef PROG_OS_SERVICES_FD_ACITIVITY_EVENT_HPP
#define PROG_OS_SERVICES_FD_ACITIVITY_EVENT_HPP

#include <concepts>

namespace prog::os_services::fd
{
	enum class activity_status
	{
		read,
		write,
		read_or_write
	};

	class activity_event
	{
	public:
		virtual activity_status get_activity_status() const noexcept = 0;
		virtual void update_listening_status(activity_status new_status) const = 0;
		virtual void close_fd() const noexcept = 0;
	};

	template<class T>
	concept activity_event_handler = requires(T& obj, activity_event const& e)
	{
		{obj.handle_event(e)} -> std::same_as<void>;
	};
}

#endif