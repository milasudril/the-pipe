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
	/**
	 * \brief Concept for an entity that can receive log items
	 *
	 * \param obj The object to test
	 * \param item The decoded log item
	 * \param ec An error code issued by the JSON parser
	 */
	template<class T>
	concept container_receiver = requires(
		T obj,
		jopp::container&& item,
		jopp::parser_error_code ec
	)
	{
		{ utils::unwrap(obj).consume(std::move(item)) } -> std::same_as<void>;
		{ utils::unwrap(obj).on_parse_error(ec) } -> std::same_as<void>;
	};

	/**
	 * \brief Abstract base class for type-erased container_receivers
	 */
	class type_erased_container_receiver
	{
	public:
		virtual ~type_erased_container_receiver() = default;

		virtual void consume(jopp::container&& item) = 0;
		virtual void on_parse_error(jopp::parser_error_code ec) = 0;
	};

	/**
	 * \brief A generic implementation of type_erased_container_receiver
	 */
	template<container_receiver ContainerReceiver>
	class container_receiver_impl:public type_erased_container_receiver
	{
	public:
		explicit container_receiver_impl(ContainerReceiver&& object):
			m_object{std::move(object)}
		{}

		void consume(jopp::container&& item) override
		{ utils::unwrap(m_object).consume(std::move(item)); }

		void on_parse_error(jopp::parser_error_code ec) override
		{ utils::unwrap(m_object).on_parse_error(ec); }

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

		/**
		 * \brief Constructs a reader
		 * \param name The name of this reader. Used for identifying the events passed to receiver
		 * \param receiver The container_receiver that will receive log items
		 * \param buffer_size The size of the internal buffer
		 */
		template<container_receiver ContainerReceiver>
		explicit reader(ContainerReceiver receiver, size_t buffer_size = 65536):
			m_buffer_size{buffer_size},
			m_input_buffer{std::make_unique<char[]>(buffer_size)},
			m_container_receiver{new container_receiver_impl(std::forward<ContainerReceiver>(receiver))},
			m_state{std::make_unique<state>()}
		{}

		/**
		 * \brief Handles file activity events
		 * \param source The activity_event_handler_store that emitted the event
		 * \param event The event to handle
		 */
		void handle_event(os_services::fd::activity_event_handler_store&, event_type const& event);

	private:
		size_t m_buffer_size;
		std::unique_ptr<char[]> m_input_buffer;
		std::unique_ptr<type_erased_container_receiver> m_container_receiver;

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