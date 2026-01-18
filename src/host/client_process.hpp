#include "src/os_services/proc_mgmt/proc_mgmt.hpp"
#include <filesystem>

namespace prog::host
{
	class client_process:os_services::proc_mgmt::process
	{
	public:
		using base = os_services::proc_mgmt::process;
		using base::pid;
		using base::file_descriptor;

		explicit client_process(
			std::filesystem::path const& client_binary,
			os_services::proc_mgmt::io_redirection const& io_redir
		):
			base{
				client_binary.c_str(),
				std::span<char const*>{},
				std::span<char const*>{},
				io_redir
			}
		{}

	private:
	};
}