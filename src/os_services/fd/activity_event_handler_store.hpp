#ifndef PIPE_OS_SERVICES_FD_ACITIVITY_EVENT_HPP
#define PIPE_OS_SERVICES_FD_ACITIVITY_EVENT_HPP

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

	class activity_event_handler_store;

	template<class CallbackTag, class FileDescriptorTag>
	struct activity_event
	{
		fd::tagged_file_descriptor_ref<FileDescriptorTag> fd;
		fd::activity_status status;
		fd::event_handler_id event_handler;
	};

	/**
	 * \brief An entity to be used to observe the state of a file descriptor
	 * \tparam T The type to query
	 * \tparam FileDescriptorTag Identifies the type of file descriptor to be used
	 */
	template<class T, class CallbackTag, class FileDescriptorTag>
	concept activity_event_handler = requires(
		T& obj,
		activity_event_handler_store& source,
		activity_event<CallbackTag, FileDescriptorTag> const& event
	)
	{
		/**
		 * \brief Will be called when the state of the file descriptor needs to be checked
		 */
		{utils::unwrap(obj).handle_event(source, event)} -> std::same_as<void>;
	};

	class activity_event_handler_store
	{
	public:
		class config_transaction
		{
		public:
			explicit config_transaction(activity_event_handler_store& monitor):
				m_monitor{monitor}
			{}

			~config_transaction()
			{
				for(auto item : m_added_ids)
				{ m_monitor.get().remove(item); }
			}

			template<class Tag, class ... Args>
			auto& add(Args&&... args)
			{
				auto const id = m_monitor.get().add<Tag>(std::forward<Args>(args)...);
				m_added_ids.push_back(id);
				return *this;
			}

			void commit()
			{ m_added_ids.clear(); }

		private:
			std::reference_wrapper<activity_event_handler_store> m_monitor;
			std::vector<event_handler_id> m_added_ids;
		};

		friend class config_transaction;

		auto make_config_transaction()
		{ return config_transaction{*this}; }

		template<class Tag>
		void update_listening_status(
			tagged_file_descriptor_ref<Tag> fd,
			activity_status new_status
		)
		{ do_update_listening_status(file_descriptor_ref{fd.native_handle()}, new_status); }

		template<
			class CallbackTag,
			class FileDescriptorTag,
			activity_event_handler<CallbackTag, FileDescriptorTag> EventHandler
		>
		[[nodiscard]] fd::event_handler_id add(
			EventHandler eh,
			tagged_file_descriptor<FileDescriptorTag> fd_to_watch,
			activity_status initial_listening_status
		)
		{
			return do_add(
				event_handler_info{
					.object_address = source_object_location{.address = &eh},
					.object_size = sizeof(EventHandler),
					.object_alignment = alignof(EventHandler),
					.handle_event = [](
						void* object,
						activity_event_handler_store& event_source,
						activity_event<void, generic_fd_tag> const& event
					){
						utils::unwrap(*static_cast<EventHandler*>(object)).handle_event(
							event_source,
							std::bit_cast<activity_event<CallbackTag, FileDescriptorTag>>(event)
						);
					},
					.destroy_event_handler_at = [](void* object){
						static_cast<EventHandler*>(object)->~EventHandler();
					},
					.construct_event_handler_at = [](
						dest_object_location dest,
						source_object_location src
					){
						::new(dest.address)EventHandler(std::move(*static_cast<EventHandler*>(src.address)));
					}
				},
				make_generic_file_descriptor(std::move(fd_to_watch)),
				initial_listening_status
			);
		}

		virtual void remove(event_handler_id id) = 0;
		virtual ~activity_event_handler_store() = default;

		struct source_object_location
		{ void* address; };

		struct dest_object_location
		{ void* address; };

		struct event_handler_info
		{
			source_object_location object_address;
			size_t object_size;
			size_t object_alignment;
			void (*handle_event)(
				void* object,
				activity_event_handler_store& event_source,
				activity_event<void, generic_fd_tag> const& event
			);
			void (*destroy_event_handler_at)(void* object);
			void (*construct_event_handler_at)(
				dest_object_location dest,
				source_object_location src
			);
		};

	private:
		virtual event_handler_id do_add(
			event_handler_info const& info,
			fd::file_descriptor fd_to_watch,
			activity_status initial_listening_status
		) = 0;

		virtual void do_update_listening_status(file_descriptor_ref fd, activity_status new_status) = 0;
	};
}

#endif