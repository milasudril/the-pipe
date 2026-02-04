#ifndef PIPE_OS_SERVICES_FD_ACITIVITY_EVENT_HPP
#define PIPE_OS_SERVICES_FD_ACITIVITY_EVENT_HPP

#include "src/log/log.hpp"
#include "src/os_services/fd/file_descriptor.hpp"
#include "src/utils/utils.hpp"
#include <concepts>

namespace Pipe::os_services::fd
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
	 * \brief The id for the current event handler
	 */
	class event_handler_id
	{
	public:
		constexpr event_handler_id() = default;

		constexpr explicit event_handler_id(uint64_t value):
			m_value{value}
		{}

		constexpr uint64_t value() const
		{ return m_value; }

		constexpr event_handler_id next()
		{
			auto ret = *this;
			++m_value;
			return ret;
		}

		constexpr bool operator==(event_handler_id const& ) const = default;

		constexpr bool operator!=(event_handler_id const& ) const = default;

	private:
		uint64_t m_value{};
	};

	struct event_handler_id_hash
	{
		static constexpr auto operator()(event_handler_id id) noexcept
		{ return std::hash<uint64_t>{}(id.value()); }
	};

	class activity_monitor;

	struct generic_activity_event
	{
	};


	class activity_monitor
	{
	public:
		template<class Tag>
		void update_listening_status(tagged_file_descriptor_ref<Tag> fd, activity_status new_status)
		{ update_listening_status(file_descriptor_ref{fd.native_handle()}, new_status); }

		virtual void remove(event_handler_id id) = 0;

	private:
		virtual void update_listening_status(file_descriptor_ref fd, activity_status new_status) = 0;

		struct source_object_location
		{
			void* address;
		};

		struct dest_target_location
		{
			void* address;
		};

		struct event_handler_info
		{
			source_object_location object_address;
			size_t size;
			size_t alignment;
			file_descriptor fd_to_watch;
			activity_status initial_listening_status;

			void (*handle_event)(
				void* object,
				activity_monitor& source,
				generic_activity_event const& event
			);

			void (*construct_event_handler_at)(
				dest_target_location dest,
				source_object_location src,
				file_descriptor fd_to_watch
			);

			void (*destroy_at)(void* object);
		};

		virtual event_handler_id add(event_handler_info const& info);
		virtual ~activity_monitor() = default;
	};

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
		{utils::unwrap(obj).handle_event(e, fd)} -> std::same_as<void>;
	};
}

#endif