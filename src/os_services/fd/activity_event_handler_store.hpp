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
	enum class activity_status:uint32_t
	{
		none = 0x0,          /**< No activity available */
		read = 0x1,          /**< Read is available */
		write = 0x2,         /**< Write is available */
		read_or_write = 0x3, /**< Read and write is available */
		error = 0x8000'0000  /**< An error has occurred on the file descriptor */
	};

	consteval void enable_bitmask_operators(activity_status){}

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

	inline constexpr bool has_error(activity_status status)
	{ return static_cast<int>(status) & static_cast<int>(activity_status::error); }

	inline constexpr char const* to_string(activity_status status)
	{
		switch (status)
		{
			case activity_status::none:
				return "none";

			case activity_status::read:
				return "read";

			case activity_status::write:
				return "write";

			case activity_status::read_or_write:
				return "read_or_write";

			case activity_status::error:
				return "error";
		}
		return "<Unknown>";
	}

	/**
	 * \brief The id for the current event handler
	 *
	 * An event_handler_id is a 64 bit running number that can be used to identify an event_handler
	 */
	class event_handler_id
	{
	public:
		constexpr event_handler_id() = default;

		/**
		 * \brief Constructs an event_handler_id from value
		 */
		constexpr explicit event_handler_id(uint64_t value):
			m_value{value}
		{}

		/**
		 * \brief Returns the value of the event_handler_id
		 */
		constexpr uint64_t value() const
		{ return m_value; }

		/**
		 * \brief Gets the current value and increments the current value (post-increment)
		 */
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

	/**
	 * \brief Hash function object for event_handler_id
	 */
	struct event_handler_id_hash
	{
		static constexpr auto operator()(event_handler_id id) noexcept
		{ return std::hash<uint64_t>{}(id.value()); }
	};

	class activity_event_handler_store;

	class saved_event_handler_ref
	{
	public:
		saved_event_handler_ref() = default;

		void* get() const
		{ return m_ptr; }

		constexpr bool operator==(saved_event_handler_ref const&) const = default;

		constexpr bool operator!=(saved_event_handler_ref const&) const = default;

	protected:
		explicit saved_event_handler_ref(void* ptr):
			m_ptr{ptr}
		{}

	private:
		void* m_ptr{nullptr};
	};

	template<class CallbackTag, class FileDescriptorTag>
	struct activity_event_handler_registered_event;

	/**
	 * \brief Describes an activity event
	 *
	 * \tparam CallbackTag Is used to distinguish between events on different protocols that use the
	 *                     same type of file descriptor
	 *
	 * \tparam FileDescriptorTag Identifies the type of file descriptor that was activated
	 */
	template<class CallbackTag, class FileDescriptorTag>
	struct activity_event;

	/**
	 * \brief An entity to be used to observe the state of a file descriptor
	 * \tparam T The type to query
	 * \tparam FileDescriptorTag Identifies the type of file descriptor to be used
	 */
	template<class T, class CallbackTag, class FileDescriptorTag>
	concept activity_event_handler = requires(
		T& obj,
		activity_event<CallbackTag, FileDescriptorTag> const& activity_event,
		activity_event_handler_registered_event<CallbackTag, FileDescriptorTag> const& activity_event_handler_registered_event
	)
	{
		/**
		 * \brief Will be called when the state of the file descriptor needs to be checked
		 */
		{utils::unwrap(obj).handle_event(activity_event)} -> std::same_as<void>;
		{utils::unwrap(obj).handle_event(activity_event_handler_registered_event)} -> std::same_as<void>;
	};

	/**
	 * \brief Stores (type-erased) activity_event_handlers
	 */
	class activity_event_handler_store
	{
	public:
		/**
		 * \brief A helper class that can be used to register multiple items, with rollback support
		 */
		class config_transaction
		{
		public:
			/**
			 * \brief Constructs a config_transaction by referencing a particular
			 *        activity_event_handler_store
			 */
			explicit config_transaction(activity_event_handler_store& store) noexcept:
				m_store{store}
			{}

			/**
			 * \brief Object destructor
			 *
			 * If the transaction was not committed, all entries successfully stored will be removed
			 */
			~config_transaction() noexcept
			{
				for(auto item : m_added_ids)
				{ m_store.get().remove(item); }
			}

			/**
			 * \brief Adds an item to the associated activity_event_handler_store
			 * \see activity_event_handler_store::add
			 */
			template<class Tag, class ... Args>
			auto& add(Args&&... args)
			{
				auto const id = m_store.get().add<Tag>(std::forward<Args>(args)...);
				m_added_ids.push_back(id);
				return *this;
			}

			/**
			 * \brief Commits all additions so they are kept when the transaction goes out of scope
			 */
			void commit() noexcept
			{ m_added_ids.clear(); }

		private:
			std::reference_wrapper<activity_event_handler_store> m_store;
			std::vector<event_handler_id> m_added_ids;
		};

		/**
		 * \brief Creates a config_transaction associated with this activity_event_handler_store
		 */
		auto make_config_transaction()
		{ return config_transaction{*this}; }

		/**
		 * \brief Adds a new entry to the activity_event_handler_store
		 * \tparam CallbackTag See activity_event
		 * \tparam FileDescriptorTag See activity_event
		 * \param eh The event handler that will be used for handling events on the file descriptor
		 * \param fd_to_watch The file descriptor that whose events will be routed to eh
		 * \param initial_listening_status The status to listen for initially
		 * \return The id of the added event handler id
		 */
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
					.handle_activity_event = [](
						void* object,
						activity_event<void, generic_fd_tag> const& event
					){
						utils::unwrap(*static_cast<EventHandler*>(object)).handle_event(
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
					},
					.handle_activity_event_handler_registered_event = [](
						void* object,
						activity_event_handler_registered_event<void, generic_fd_tag> const& event
					){
						utils::unwrap(*static_cast<EventHandler*>(object)).handle_event(
							std::bit_cast<activity_event_handler_registered_event<CallbackTag, FileDescriptorTag>>(event)
						);
					}
				},
				make_generic_file_descriptor(std::move(fd_to_watch)),
				initial_listening_status
			);
		}

		/**
		 * \brief Removes the event handler that corresponds to id
		 */
		virtual void remove(event_handler_id id) noexcept = 0;

		virtual void update_listening_status(saved_event_handler_ref handle, activity_status new_status) noexcept = 0;

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
			void (*handle_activity_event)(
				void* object,
				activity_event<void, generic_fd_tag> const& event
			);
			void (*destroy_event_handler_at)(void* object);
			void (*construct_event_handler_at)(
				dest_object_location dest,
				source_object_location src
			);
			void (*handle_activity_event_handler_registered_event)(
				void* object,
				activity_event_handler_registered_event<void, generic_fd_tag> const& event
			);
		};

	private:
		virtual event_handler_id do_add(
			event_handler_info const& info,
			fd::file_descriptor fd_to_watch,
			activity_status initial_listening_status
		) = 0;
	};

	template<class CallbackTag, class FileDescriptorTag>
	struct activity_event_handler_registered_event
	{
		bool is_valid() const
		{ return fd != nullptr; }

		tagged_file_descriptor_ref<FileDescriptorTag> fd;
		event_handler_id id;
		saved_event_handler_ref event_handler;
		activity_event_handler_store* event_handler_store{nullptr};

		constexpr bool operator==(activity_event_handler_registered_event const&) const = default;
		constexpr bool operator!=(activity_event_handler_registered_event const&) const = default;
	};

	template<class CallbackTag, class FileDescriptorTag>
	struct activity_event
	{
		/**
		 * \brief The current status of the associated file descriptor
		 */
		activity_status status;
	};
}

#endif