#ifndef PROG_OS_SERVICES_FD_ACITIVITY_EVENT_HPP
#define PROG_OS_SERVICES_FD_ACITIVITY_EVENT_HPP

#include "src/os_services/fd/file_descriptor.hpp"
#include <concepts>

namespace prog::os_services::fd
{
	enum class activity_status
	{
		none = 0x0,
		read = 0x1,
		write = 0x2,
		read_or_write = 0x3
	};

	inline constexpr bool can_read(activity_status status)
	{
		return static_cast<int>(status) & static_cast<int>(activity_status::read);
	}

	inline constexpr bool can_write(activity_status status)
	{
		return static_cast<int>(status) & static_cast<int>(activity_status::write);
	}

	class activity_event
	{
	public:
		virtual activity_status get_activity_status() const noexcept = 0;
		virtual void update_listening_status(activity_status new_status) const = 0;
		virtual void close_fd() const noexcept = 0;
	};

	template<class T, class FileDescriptorTag>
	concept activity_event_handler = requires(
		T& obj,
		activity_event const& e,
		tagged_file_descriptor_ref<FileDescriptorTag> fd
	)
	{
		{obj.handle_event(e, fd)} -> std::same_as<void>;
	};
}

#endif