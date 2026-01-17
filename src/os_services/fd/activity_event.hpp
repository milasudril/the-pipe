#ifndef PROG_OS_SERVICES_FD_ACITIVITY_EVENT_HPP
#define PROG_OS_SERVICES_FD_ACITIVITY_EVENT_HPP

#include "src/os_services/fd/file_descriptor.hpp"
#include <concepts>

namespace prog::os_services::fd
{
	/**
	 * \brief Describes the type of activity that is currently possible on a file descriptor
	 */
	enum class activity_status
	{
		none = 0x0,         /**< No activity available */
		read = 0x1,         /**< Read is available */
		write = 0x2,        /**< Write is available */
		read_or_write = 0x3 /**< Read and write is available */
	};

	/**
	 * \brief Checks whether or not the read flag has been set in status
	 */
	inline constexpr bool can_read(activity_status status)
	{ return static_cast<int>(status) & static_cast<int>(activity_status::read); }

	/**
	 * \brief Checks whether or not the write flag has been set in status
	 */
	inline constexpr bool can_write(activity_status status)
	{ return static_cast<int>(status) & static_cast<int>(activity_status::write); }

	/**
	 * \brief Describes an activity event (possibly fired by activity_monitor)
	 */
	class activity_event
	{
	public:
		/**
		 * \brief Gets the activity_status for this event
		 */
		virtual activity_status get_activity_status() const noexcept = 0;

		/**
		 * \brief Updates the listening status for the resource that fired this event
		 */
		virtual void update_listening_status(activity_status new_status) const = 0;

		/**
		 * \brief Stops listening and cleans up resources associated with this event
		 */
		virtual void stop_listening() const noexcept = 0;
	};

	/**
	 * \brief An entity to be used to observe the state of a file descriptor
	 * \tparam T The type to query
	 * \tparam FileDescriptorTag Identifies the type of file descriptor to be used
	 */
	template<class T, class FileDescriptorTag>
	concept activity_event_handler = requires(
		T& obj,
		activity_event const& e,
		tagged_file_descriptor_ref<FileDescriptorTag> fd
	)
	{
		/**
		 * \brief Will be called when the state of the file descriptor needs to be checked
		 */
		{obj.handle_event(e, fd)} -> std::same_as<void>;
	};
}

#endif