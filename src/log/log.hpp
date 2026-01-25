//@	{"dependencies_extra":[{"ref":"./log.o", "rel":"implementation"}]}

#ifndef PIPE_LOG_HPP
#define PIPE_LOG_HPP

#include <chrono>
#include <format>
#include <functional>
#include <string_view>

/**
 * \brief Logging facilities
 */
namespace Pipe::log
{
	/**
	 * \brief The severity of o a log message
	 */
	enum class severity{info, warning, error};

	/**
	 * \brief The type of clock to be used for log items
	 */
	using clock = std::chrono::system_clock;

	/**
	 * An item in a log
	 */
	struct item
	{
		clock::time_point when;
		enum severity severity;
		std::string message;
	};

	/**
	 * \brief Describes the requirement of a writer
	 */
	template<class T>
	concept writer = requires (T& obj, item&& item_to_write) {
		{ obj.write(std::move(item_to_write)) } -> std::same_as<void>;
	};

	/**
	 * \brief Writes item_to_write using writer
	 */
	template<writer T>
	void write_message(item&& item_to_write, T& writer)
	{ writer.write(std::move(item_to_write)); }

	class type_erased_writer
	{
	public:
		type_erased_writer() = default;

		template<writer T>
		type_erased_writer(std::reference_wrapper<T> object):
			m_object{&object.get()},
			m_write{[](void* writer, item&& item_to_write) {
				static_cast<T*>(writer)->write(std::move(item_to_write));
			}}
		{}

		void write(item&& item)
		{ m_write(m_object, std::move(item)); }

	private:
		void* m_object = nullptr;
		static void null_write(void*, item&&) {}
		void (*m_write)(void*, item&&) = null_write;
	};

	/**
	 * \brief Describes the requirement of a timestamp_generator
	 */
	template<class T>
	concept timestamp_generator = requires(T& x) {
		{ x.now() } -> std::same_as<clock::time_point>;
	};

	class type_erased_timestamp_generator
	{
	public:
		type_erased_timestamp_generator() = default;

		template<timestamp_generator T>
		type_erased_timestamp_generator(std::reference_wrapper<T> object):
			m_object{&object.get()},
			m_now{[](void* timestamp_generator) {
				return static_cast<T*>(timestamp_generator)->now();
			}}
		{}

		clock::time_point now()
		{ return m_now(m_object); }

	private:
		void* m_object = nullptr;
		static clock::time_point null_now(void*)
		{ return clock::time_point{}; }

		clock::time_point (*m_now)(void*) = null_now;
	};

	/**
	 * \brief Writes a pre-formatted log message using the current writer
	 */
	void write_message(enum severity severity, std::string&& message);

	/**
	 * \brief Formats a log message and writes it using the current writer
	 */
	template<class ... Args>
	void write_message(enum severity severity, std::format_string<Args...> fmt, Args... args) noexcept
	{
		try
		{ write_message(severity, std::format(fmt, std::forward<Args>(args)...)); }
		catch(...)
		{ abort(); }
	}

	/**
	 * \brief Describes the configuration of a logger
	 */
	struct configuration
	{
		type_erased_writer writer;
		type_erased_timestamp_generator timestamp_generator;
	};

	/**
	 * \brief Configures the global log
	 */
	void configure(configuration const& log_cfg) noexcept;
};

#endif