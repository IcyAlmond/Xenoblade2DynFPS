#pragma once

#include <lib/log/svc_logger.hpp>
#include <string>

/* Specify logger implementations here. */
inline exl::log::LoggerMgr<
    exl::log::SvcLogger
> Logging;


inline void Log(const std::string& str) {
	Logging.Log("MYMOD: " + str);
}
