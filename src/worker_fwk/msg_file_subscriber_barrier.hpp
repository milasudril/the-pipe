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

		void inc_num_subscribers() noexcept
		{ ++m_num_subscribers; }

		void dec_num_subscribers()
		{
			m_num_subscribers = m_num_subscribers > 0 ? m_num_subscribers - 1: 0;
			m_num_ready_subscribers = std::min(m_num_subscribers, m_num_ready_subscribers);
			submit_result_if_ready();
		}

		void inc_num_ready_subscribers()
		{
			++m_num_ready_subscribers;
			submit_result_if_ready();
		}

		auto get_num_subscribers() const  noexcept
		{ return m_num_subscribers; }

		auto get_num_ready_subscribers() const noexcept
		{ return m_num_ready_subscribers; }

		bool is_bound_to(submit_function other_submit_function) const noexcept
		{ return m_submit_result == other_submit_function; }

	private:
		void submit_result_if_ready()
		{
			if(m_num_ready_subscribers == m_num_subscribers && m_num_subscribers != 0)
			{
				m_submit_result();
				m_num_ready_subscribers = 0;
			}
		}

		size_t m_num_subscribers{};
		size_t m_num_ready_subscribers{};
		submit_function m_submit_result;
	};
}

#endif