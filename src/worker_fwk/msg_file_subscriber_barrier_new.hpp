#ifndef PIPE_WORKER_FWK_MSG_FILE_SUBSCRIBER_BARRIER_HPP
#define PIPE_WORKER_FWK_MSG_FILE_SUBSCRIBER_BARRIER_HPP

#include "src/utils/bound_member_function.hpp"

#include <cstddef>
#include <algorithm>

namespace Pipe::worker_fwk
{
	class msg_file_subscriber_barrier
	{
	public:
		using submit_function = utils::bound_member_function<void>;

		explicit msg_file_subscriber_barrier(submit_function submit_result) noexcept:
			m_submit_result{submit_result}
		{}

		void dec_num_busy_subscriberes() noexcept
		{
			--m_num_busy_subscibers;
			submit_result_if_ready();
		}

		void inc_num_busy_subscribers() noexcept
		{
			++m_num_busy_subscibers;
		}

		bool is_bound_to(submit_function other_submit_function) const noexcept
		{ return m_submit_result == other_submit_function; }

	private:
		void submit_result_if_ready()
		{
			if(m_num_busy_subscibers == 0)
			{ m_submit_result(); }
		}

		size_t m_num_busy_subscibers=0;
		submit_function m_submit_result;
	};
}

#endif