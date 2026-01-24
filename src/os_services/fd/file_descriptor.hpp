#ifndef PIPE_OS_SERVICE_FD_FILE_DESCRIPTOR_HPP
#define PIPE_OS_SERVICE_FD_FILE_DESCRIPTOR_HPP

#include <unistd.h>
#include <memory>
#include <cassert>

namespace Pipe::os_services::fd
{
	/**
	 * \brief Struct to specialize to enable conversions from a \ref tagged_file_descriptor_ref "tagged_file_descriptor_ref<Tag>"
	 *        to another tagged_file_descriptor_ref.
	 *
	 * To enable conversion from `foo` to `bar`, do the following
	 *
	 * ```
	 * //bar and foo should have been declared earlier
	 *
	 * template<>
	 * struct Pipe::os_services::fd::enabled_fd_conversions<foo>
	 * {
	 * 	static consteval void supports(bar){}
	 * };
	 * ```
	 */
	template<class Tag>
	struct enabled_fd_conversions
	{};

	/**
	 * \brief Class referring to a file descriptor
	 * \tparam Tag An arbitrary type that can be used to identify the kind of file descriptor
	 *             the object refers to, such as an open file or socket.
	 */
	template<class Tag>
	class tagged_file_descriptor_ref
	{
	public:
		/**
		 * \brief The tag type, stored for introspection purposes
		 */
		using tag_type = Tag;
		using native_handle_type = int;

		tagged_file_descriptor_ref() = default;

		/**
		 * \name Converting constructors
		 */
		//@{
		/**
		 * \brief Constructs a tagged_file_descriptor_ref with the file descriptor given by fd
		 * \post native_handle will return fd
		 */
		tagged_file_descriptor_ref(int fd) noexcept: m_ref{fd}
		{ }

		/**
		 * \brief Constructs a tagged_file_descriptor_ref from nullptr_t
		 * \post Tis valid will return false
		 */
		tagged_file_descriptor_ref(std::nullptr_t) noexcept: m_ref{-1} {}
		//@}

		/**
		 * \brief Returns the underlying file descriptor
		 */
		[[nodiscard]] auto native_handle() const noexcept
		{ return m_ref; }

		bool operator==(tagged_file_descriptor_ref const& other) const = default;
		bool operator!=(tagged_file_descriptor_ref const& other) const = default;
		bool operator==(std::nullptr_t) const noexcept
		{ return m_ref == -1;}
		bool operator!=(std::nullptr_t) const noexcept
		{ return m_ref != -1; }

		/**
		 * \brief Checks whether or not the referenced file descriptor is valid
		 */
		[[nodiscard]] auto is_valid() const noexcept
		{ return m_ref != -1; }

		/**
		 * \brief Operator to make tagged_file_descriptor_ref objects de-referenceable
		 */
		[[nodiscard]] auto operator*() const noexcept
		{ return m_ref; }

		/**
		 * \brief Operator to convert to the native handle type
		 */
		[[nodiscard]] operator native_handle_type() const noexcept
		{ return native_handle(); }

		/**
		 * \brief Enables conversion to another type of file descriptor
		 *
		 * \note Specialize enabled_fd_conversions<Tag>::supports to enable conversion to a specific
		 * type of file descriptor
		 */
		template<class OtherTag>
		requires requires{{enabled_fd_conversions<Tag>::supports(std::declval<OtherTag>())};}
		operator tagged_file_descriptor_ref<OtherTag>() const noexcept
		{ return tagged_file_descriptor_ref<OtherTag>{m_ref}; }

	private:
		native_handle_type m_ref{-1};
	};

	template<class Tag>
	[[nodiscard]] inline bool operator==(std::nullptr_t, tagged_file_descriptor_ref<Tag> other)
	{ return other == nullptr; }

	template<class Tag>
	[[nodiscard]] inline bool operator!=(std::nullptr_t, tagged_file_descriptor_ref<Tag> other)
	{ return other != nullptr; }

	/**
	 * \brief A deleter for file descriptors
	 * \tparam Tag An arbitrary type that can be used to identify the kind of file descriptor
	 *             the object refers to, such as an open file or socket.
	 * \note The user may specialize file_descriptor_deleter depending on special needs. For example,
	 *       a specialization for handling sockets may wish to call shutdown before close.
	 */
	template<class Tag>
	struct file_descriptor_deleter
	{
		/**
		 * \brief The "pointer" type to "delete"
		 */
		using pointer = tagged_file_descriptor_ref<Tag>;

		/**
		 * \brief Function call operator that implements the delete operation
		 */
		static void operator()(tagged_file_descriptor_ref<Tag> fd) noexcept
		{
			if(fd.native_handle() != -1)
			{ close(fd.native_handle()); }
		}
	};

	/**
	 * \brief An owner of a file descriptor
	 * \tparam Tag An arbitrary type that can be used to identify the kind of file descriptor
	 *             the object refers to, such as an open file or socket.
	 */
	template<class Tag>
	using tagged_file_descriptor = std::unique_ptr<
		tagged_file_descriptor_ref<Tag>,
		file_descriptor_deleter<Tag>
	>;

	/**
	 * \brief A tag type for an arbitrary file descriptor
	 */
	struct generic_fd_tag
	{};

	/**
	 * \brief A reference to an arbitrary file descriptor
	 */
	using file_descriptor_ref = tagged_file_descriptor_ref<generic_fd_tag>;

	/**
	 * \brief An owner of an arbitrary file descriptor
	 */
	using file_descriptor = tagged_file_descriptor<generic_fd_tag>;
}

#endif