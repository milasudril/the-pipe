#ifndef PIPE_WORKER_FWK_MSG_FILE_OUTPUT_PORT_HPP
#define PIPE_WORKER_FWK_MSG_FILE_OUTPUT_PORT_HPP

#include "src/utils/bound_member_function.hpp"

#include <cassert>
#include <utility>
#include <flat_set>

namespace Pipe::worker_fwk
{
	template<class T>
	concept msg_file_output_port_subscription = requires(T& obj)
	{
		{ obj.notify_data_ready() } -> std::same_as<void>;
		{ std::as_const(obj).is_busy() } -> std::same_as<bool>;
	};

	class msg_file_output_port_subscription_ref
	{
	public:
		template<msg_file_output_port_subscription_ref Obj>
		explicit msg_file_output_port_subscription_ref(std::reference_wrapper<Obj> obj):
			m_handle{&obj.get()},
			m_notify_data_ready{
				[](void* obj){
					static_cast<Obj*>(obj)->notify_data_ready();
				}
			},
			m_is_busy{
				[](void const* obj) {
					static_cast<Obj const*>(obj)->is_busy();
				}
			}
		{}

		void notify_data_ready() const
		{ m_notify_data_ready(m_handle); }

		bool is_busy() const
		{ m_is_busy(m_handle); }

		auto operator<=>(msg_file_output_port_subscription_ref const& other) const
		{ return m_handle <=> other.m_handle; }


	private:
		void* m_handle;
		void (*m_notify_data_ready)(void*);
		void (*m_is_busy)(void const*);
	};

	class msg_file_output_port
	{
	public:
		explicit msg_file_output_port(utils::bound_member_function<void> submit_callback):
			m_submit_callback{submit_callback}
		{}

		void dec_num_busy_subscriberes()
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

		void add_subscription(msg_file_output_port_subscription_ref subscription) __restrict__
		{
			m_subscriptions.insert(subscription);
			submit_results_if_ready();
		}

		bool remove_subscription(msg_file_output_port_subscription_ref subscription)
		{
			auto ret = m_subscriptions.erase(subscription);
			if(ret == 0)
			{ return false; }

			if(unwrap(subscription).is_busy())
			{
				--m_num_busy_subscribers;
				submit_results_if_ready();
			}
			return true;
		}

	private:
		utils::bound_member_function<void> m_submit_callback;
		size_t m_num_busy_subscribers{0};
		std::flat_set<msg_file_output_port_subscription_ref> m_subscriptions;
		bool m_result_submitted{false};
	};
}

#endif