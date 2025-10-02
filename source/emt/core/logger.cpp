#include "logger.h"
#include "config.h"
#include <cstdio>
#include <cstdarg>
#include <cstdlib>

namespace emt
{

const char* level_color(logger::level level)
{
	switch(level) {
		case logger::info:
			return "\033[38;5;208m";
		case logger::debug:
			return "\033[38;5;117m";
		case logger::warn:
			return "\033[33m";
		case logger::error:
			return "\033[32m";
		default:
			return "\033[0m";
	}
}

const char* level_string(logger::level level)
{
	switch(level) {
		case logger::info:
			return "info";
		case logger::debug:
			return "debug";
		case logger::warn:
			return "WARN";
		case logger::error:
			return "ERROR";
		default:
			return "NONE";
	}
}

void enable_win_console_ansi_support()
{
	static bool ansi_support = false;
	if(ansi_support)
		return;
#ifdef _WIN32
	HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
	if(handle == INVALID_HANDLE_VALUE)
		return;

	DWORD mode = 0;
	if(!GetConsoleMode(handle, &mode))
		return;

	mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
	SetConsoleMode(handle, mode);
#endif
	ansi_support = true;
}

logger::logger()
{
	enable_win_console_ansi_support();
}

void logger::log(level level, const char* code, ...)
{
	char         buffer[1024];
	std::va_list args;
	va_start(args, code);
	std::vsnprintf(buffer, sizeof(buffer), code, args);
	va_end(args);

	auto ansi_color = level_color(level);
	auto level_str  = level_string(level);

	std::printf("[%s%s\033[0m] %s\n", ansi_color, level_str, buffer);
}

void logger::log_assert_t(const char* msg, const char* file, int line)
{
	char buf[1024];
	std::snprintf(buf, sizeof(buf),
	              "[assert] \n"
	              "Message : %s\n"
	              "File    : %s\n"
	              "Line    : %d",
	              msg ? msg : "no message",
	              file, line);

	log_error("%s\n", buf);
#ifdef _WIN32
	OutputDebugStringA(buf);
	OutputDebugStringA("\n");
	MessageBoxA(nullptr, buf, "Assertion Failed",
	            MB_ICONERROR | MB_OK | MB_TASKMODAL | MB_TOPMOST);
#endif
	std::exit(EXIT_FAILURE);
}

}        // namespace emt
