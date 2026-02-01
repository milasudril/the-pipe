//@	{"dependencies_extra": [{"ref": "./reader.o", "rel":"implementation"}]}

#ifndef PIPE_JSON_LOG_READER_HPP
#define PIPE_JSON_LOG_READER_HPP

#include "src/log/log.hpp"
#include "src/os_services/io/io.hpp"
#include "src/os_services/fd/activity_event.hpp"
#include "src/utils/utils.hpp"

#include <jopp/types.hpp>
#include <jopp/parser.hpp>
#include <memory>

namespace Pipe::json_log
{
	/**
	 * \brief Concept for an entity that can receive log items
	 */
	template<class T>
	concept item_receiver = requires(
		T obj,
		char const* who,
		log::item&& item,
		jopp::parser_error_code ec,
		char const* msg
	)
	{
		{ utils::unwrap(obj).consume(who, std::move(item)) } -> std::same_as<void>;
		{ utils::unwrap(obj).on_parse_error(who, ec) } -> std::same_as<void>;
		{ utils::unwrap(obj).on_invalid_log_item(who, msg) } -> std::same_as<void>;
	};

	class type_erased_item_receiver
	{
	public:
		virtual ~type_erased_item_receiver() = default;
		virtual void consume(char const* who, log::item&& item) = 0;
		virtual void on_parse_error(char const* who, jopp::parser_error_code ec) = 0;
		virtual void on_invalid_log_item(char const* who, char const* message) = 0;
	};

	template<item_receiver ItemReceiver>
	class item_receiver_impl:public type_erased_item_receiver
	{
	public:
		explicit item_receiver_impl(ItemReceiver&& object):
			m_object{std::move(object)}
		{}

		void consume(char const* who, log::item&& item) override
		{ utils::unwrap(m_object).consume(who, std::move(item)); }

		void on_parse_error(char const* who, jopp::parser_error_code ec) override
		{ utils::unwrap(m_object).on_parse_error(who, ec); }

		void on_invalid_log_item(char const* who, char const* message) override
		{ utils::unwrap(m_object).on_invalid_log_item(who, message); }

	private:
		ItemReceiver m_object;
	};

	class reader
	{
	public:
		template<item_receiver ItemReceiver>
		explicit reader(std::string&& who, ItemReceiver receiver, size_t buffer_size = 65536):
			m_buffer_size{buffer_size},
			m_input_buffer{std::make_unique<char[]>(buffer_size)},
			m_item_receiver{new item_receiver_impl(std::forward<ItemReceiver>(receiver))},
			m_who{std::move(who)},
			m_state{std::make_unique<state>()}
		{}

		void handle_event(
			os_services::fd::activity_event const& event,
			os_services::io::input_file_descriptor_ref fd
		);

	private:
		size_t m_buffer_size;
		std::unique_ptr<char[]> m_input_buffer;
		std::unique_ptr<type_erased_item_receiver>  m_item_receiver;
		std::string m_who;

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