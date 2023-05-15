#include "scpch.h"
#include "Logger.h"

#include <spdlog/async.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/msvc_sink.h>
#include <iostream>
using namespace SC;


Logger::Logger()
{
}

void Logger::CreateConsole()
{
	//if (AllocConsole())
	//{
		spdlog::init_thread_pool(8192, 1);

		auto stdSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
		auto rotSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>("logs/log.txt", 1024 * 1024 * 5, 3, true);
		auto msvcSink = std::make_shared<spdlog::sinks::msvc_sink_mt>();

		std::vector<spdlog::sink_ptr> sinks{ stdSink, rotSink, msvcSink };

		m_Logger = std::make_shared<spdlog::async_logger>("Video Streaming", sinks.begin(), sinks.end(), spdlog::thread_pool(), spdlog::async_overflow_policy::block);

		spdlog::register_logger(m_Logger);
		spdlog::set_default_logger(m_Logger);
	//}
}