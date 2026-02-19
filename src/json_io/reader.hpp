//@	{"dependencies_extra": [{"ref": "./reader.o", "rel":"implementation"}]}

#ifndef PIPE_JSON_IO_READER_HPP
#define PIPE_JSON_IO_READER_HPP

#include "src/os_services/io/io.hpp"
#include "src/os_services/fd/activity_event_handler_store.hpp"

#include <jopp/types.hpp>
#include <jopp/parser.hpp>
#include <memory>

namespace Pipe::json_io
{
	template<class Tag>
	struct container_loaded_event
	{
		jopp::container obj;
	};

	template<class Tag>
	struct parser_error_event
	{
		jopp::parser_error_code ec;
	};

	/**
	 * \brief Concept for an entity that can receive log items
	 *
	 * \param obj The object to test
	 * \param item The decoded log item
	 * \param ec An error code issued by the JSON parser
	 */
	template<class T, class Tag>
	concept container_receiver = requires(
		T obj,
		container_loaded_event<Tag>&& item,
		parser_error_event<Tag> ec
	)
	{
		{ utils::unwrap(obj).handle_event(std::move(item)) } -> std::same_as<void>;
		{ utils::unwrap(obj).handle_event(ec) } -> std::same_as<void>;
	};

	/**
	 * \brief Abstract base class for type-erased container_receivers
	 */
	class type_erased_container_receiver
	{
	public:
		virtual ~type_erased_container_receiver() = default;

		virtual void handle_event(jopp::container&& item) = 0;
		virtual void handle_event(jopp::parser_error_code ec) = 0;
	};

	/**
	 * \brief A generic implementation of type_erased_container_receiver
	 */
	template<class Tag, container_receiver<Tag> ContainerReceiver>
	class container_receiver_impl:public type_erased_container_receiver
	{
	public:
		explicit container_receiver_impl(ContainerReceiver&& object):
			m_object{std::move(object)}
		{}

		void handle_event(jopp::container&& item) override
		{ utils::unwrap(m_object).handle_event(container_loaded_event<Tag>{std::move(item)});}

		void handle_event(jopp::parser_error_code ec) override
		{ utils::unwrap(m_object).handle_event(parser_error_event<Tag>{ec}); }

	private:
		ContainerReceiver m_object;
	};

	/**
	 * \brief A reader that decodes JSON containers
	 * \note A reader can be used as a listener in os_services::fd::activity_event_handler_store
	 */
	class reader
	{
	public:
		struct json_stream_tag{};
		using event_type = os_services::fd::activity_event<
			json_stream_tag,
			os_services::io::input_file_descriptor_tag
		>;

		using activity_event_handler_registered_event = os_services::fd::activity_event_handler_registered_event<
			json_stream_tag,
			os_services::io::input_file_descriptor_tag
		>;

		/**
		 * \brief Constructs a reader
		 * \param name The name of this reader. Used for identifying the events passed to receiver
		 * \param receiver The container_receiver that will receive log items
		 * \param buffer_size The size of the internal buffer
		 */
		template<container_receiver<void> ContainerReceiver>
		explicit reader(ContainerReceiver receiver, size_t buffer_size = 65536):
			m_buffer_size{buffer_size},
			m_input_buffer{std::make_unique<char[]>(buffer_size)},
			m_container_receiver{
				new container_receiver_impl<void, ContainerReceiver>(
					std::forward<ContainerReceiver>(receiver)
				)
			},
			m_state{std::make_unique<state>()}
		{}

		/**
		 * \brief Constructs a reader
		 * \param name The name of this reader. Used for identifying the events passed to receiver
		 * \param receiver The container_receiver that will receive log items
		 * \param buffer_size The size of the internal buffer
		 */
		template<class Tag, container_receiver<Tag> ContainerReceiver>
		explicit reader(Tag, ContainerReceiver receiver, size_t buffer_size = 65536):
			m_buffer_size{buffer_size},
			m_input_buffer{std::make_unique<char[]>(buffer_size)},
			m_container_receiver{
				new container_receiver_impl<Tag, ContainerReceiver>(
					std::forward<ContainerReceiver>(receiver)
				)
			},
			m_state{std::make_unique<state>()}
		{}

		/**
		 * \brief Handles file activity events
		 * \param source The activity_event_handler_store that emitted the event
		 * \param event The event to handle
		 */
		void handle_event(os_services::fd::activity_event_handler_store&, event_type const& event);

		void handle_event(
			activity_event_handler_registered_event const& event
		)
		{
			m_registration = event;
		}

	private:
		size_t m_buffer_size;
		std::unique_ptr<char[]> m_input_buffer;
		std::unique_ptr<type_erased_container_receiver> m_container_receiver;
		activity_event_handler_registered_event m_registration;

		struct state
		{
			state():parser{container}{}

			jopp::container container;
			jopp::parser parser;
		};

		std::unique_ptr<state> m_state;
	};
}

#endif