#pragma once

#define log_info(...)  emt::logger::instance()->log(emt::logger::info, __VA_ARGS__)
#define log_debug(...) emt::logger::instance()->log(emt::logger::debug, __VA_ARGS__)
#define log_warn(...)  emt::logger::instance()->log(emt::logger::warn, __VA_ARGS__)
#define log_error(...) emt::logger::instance()->log(emt::logger::error, __VA_ARGS__)
#define log_assert(cond, msg)                                    \
	do {                                                         \
		if(!(cond)) {                                            \
			emt::logger::log_assert_t(msg, __FILE__, __LINE__); \
		}                                                        \
	} while(0)

namespace emt
{
class logger
{
public:
	enum level {
		info,
		debug,
		warn,
		error
	};
	inline static logger* instance()
	{
		static logger instance;
		return &instance;
	}
	void        log(level level, const char* code, ...);
	static void log_assert_t(const char* msg, const char* file, int line);

private:
	logger();
	~logger() = default;

	logger(const logger&)            = delete;
	logger& operator=(const logger&) = delete;
};
}        // namespace emt
