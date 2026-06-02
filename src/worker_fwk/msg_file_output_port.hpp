#ifndef PIPE_WORKER_FWK_MSG_FILE_OUTPUT_PORT_HPP
#define PIPE_WORKER_FWK_MSG_FILE_OUTPUT_PORT_HPP

#include "src/utils/bound_member_function.hpp"
#include "src/utils/unwrap.hpp"

#include <cassert>
#include <flat_map>
#include <functional>

namespace Pipe::worker_fwk
{
	template<class T>
	concept msg_file_output_port_subscription = requires(T& obj)
	{
		{ obj.notify_data_ready() } -> std::same_as<void>;
		{ obj.is_busy() } -> std::same_as<bool>;
	};

	template<class T>
	requires(
		utils::reftype<T> &&
		msg_file_output_port_subscription<decltype(utils::unwrap(std::declval<T>()))>
	)
	class msg_file_output_port
	{
	public:
		enum class subscriber_state:bool{ready = false, busy = true};

		explicit msg_file_output_port(utils::bound_member_function<void> submit_callback):
			m_submit_callback{submit_callback}
		{}

		void dec_num_busy_subscribers(T const& subscription)
		{
			auto const i = m_subscriptions.find(subscription);
			if(i == std::end(m_subscriptions))
			{ throw std::runtime_error{"The subscription does not exist on this port"}; }

			if(i->second == subscriber_state::ready)
			{ throw std::runtime_error{"The subscriber has already released this resource"}; }

			i->second = subscriber_state::ready;

			assert(m_num_busy_subscribers != 0);
			--m_num_busy_subscribers;
			submit_results_if_ready();
		}

		void submit_results_if_ready()
		{
			if(m_num_busy_subscribers == 0 && !m_result_submitted && !m_subscriptions.empty())
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
				utils::unwrap(item.first).notify_data_ready();
				item.second = subscriber_state::busy;
				++m_num_busy_subscribers;
			}
		}

		void add_subscription(T const& subscription) __restrict__
		{
			m_subscriptions.insert(std::pair{subscription, subscriber_state::ready});
			submit_results_if_ready();
		}

		void remove_subscription(T const& subscription)
		{
			auto const i = m_subscriptions.find(subscription);
			if(i == std::end(m_subscriptions))
			{ throw std::runtime_error{"The subscription does not exist on this port"}; }

			if(i->second == subscriber_state::busy)
			{
				assert(m_num_busy_subscribers != 0);
				--m_num_busy_subscribers;
				submit_results_if_ready();
			}

			m_subscriptions.erase(i);
		}

	private:
		utils::bound_member_function<void> m_submit_callback;
		size_t m_num_busy_subscribers{0};
		std::flat_map<T, subscriber_state> m_subscriptions;
		bool m_result_submitted{false};
	};
}

#endif