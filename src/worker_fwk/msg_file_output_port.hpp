#ifndef PIPE_WORKER_FWK_MSG_FILE_OUTPUT_PORT_HPP
#define PIPE_WORKER_FWK_MSG_FILE_OUTPUT_PORT_HPP

#include "src/utils/bound_member_function.hpp"
#include "src/utils/unwrap.hpp"

#include <cassert>
#include <utility>
#include <flat_set>
#include <functional>

namespace Pipe::worker_fwk
{
	template<class T>
	concept msg_file_output_port_subscription = requires(T& obj)
	{
		{ obj.notify_data_ready() } -> std::same_as<void>;
		{ std::as_const(obj).is_busy() } -> std::same_as<bool>;
	};

	template<class T>
	requires(
		utils::reftype<T> &&
		msg_file_output_port_subscription<decltype(utils::unwrap(std::declval<T>()))>
	)
	class msg_file_output_port
	{
	public:
		explicit msg_file_output_port(utils::bound_member_function<void> submit_callback):
			m_submit_callback{submit_callback}
		{}

		void dec_num_busy_subscribers()
		{
			assert(m_num_busy_subscribers != 0);
			--m_num_busy_subscribers;
			submit_results_if_ready();
		}

		void submit_results_if_ready()
		{
			if(m_num_busy_subscribers == 0)
			{
				m_submit_callback();
				m_result_submitted = true;
			}
		}

		void notify_data_ready() __restrict__
		{
			m_result_submitted = false;
			for(auto item:m_subscriptions)
			{
				item.notify_data_ready();
				++m_num_busy_subscribers;
			}
		}

		void add_subscription(T const& subscription) __restrict__
		{
			m_subscriptions.insert(subscription);
			submit_results_if_ready();
		}

		bool remove_subscription(T const& subscription)
		{
			auto ret = m_subscriptions.erase(subscription);
			if(ret == 0)
			{ return false; }

			if(subscription.is_busy())
			{ dec_num_busy_subscribers(); }
			return true;
		}

	private:
		utils::bound_member_function<void> m_submit_callback;
		size_t m_num_busy_subscribers{0};
		std::flat_set<T> m_subscriptions;
		bool m_result_submitted{false};
	};
}

#endif