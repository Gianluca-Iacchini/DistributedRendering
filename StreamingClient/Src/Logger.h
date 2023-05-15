#pragma once

#include <spdlog/spdlog.h>

namespace SC {
	class Logger
	{
	public:
		Logger();
		void CreateConsole();

		/* Wrapper functions for spdlog */
		template<typename T>
		void Info(const T& msg) { m_Logger->info(msg); }

		template<typename... Args>
		void Info(spdlog::format_string_t<Args...> fmt, Args &&... args)
		{
			m_Logger->info(fmt, args...);
		}

		template<typename T>
		void Warn(const T& msg) { m_Logger->warn(msg); }

		template<typename T>
		void Error(const T& msg) { m_Logger->error(msg); }

		template<typename... Args>
		void Error(spdlog::format_string_t<Args...> fmt, Args &&... args)
		{
			m_Logger->error(fmt, args...);
		}
	private:

		std::shared_ptr<spdlog::logger> m_Logger;
	};
}

