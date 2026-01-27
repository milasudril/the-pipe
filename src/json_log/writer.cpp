//@	{"target": {"name": "writer.o"}}

#include "./writer.hpp"
#include "./item_converter.hpp"

#include <jopp/serializer.hpp>

void Pipe::json_log::writer::write(log::item const& item)
{
	auto const object = to_jopp_object(item);
	jopp::serializer serializer{object};
	std::span current_range{m_output_buffer.get(), m_buffer_size};
	while(true)
	{
		auto const serialize_result = serializer.serialize(current_range);
		std::span data_to_write(std::data(current_range), serialize_result.ptr);
		while(std::size(data_to_write) != 0)
		{
			// Assume writing to log will never block
			auto const write_result = os_services::io::write(m_output_fd, std::as_bytes(data_to_write));
			data_to_write = std::span{
				std::begin(data_to_write) + write_result.bytes_transferred(), std::end(data_to_write)
			};
		}
		if(serialize_result.ec == jopp::serializer_error_code::completed)
		{ return; }
	}
}