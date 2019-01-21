/*
 * BNC SDN Controller GPL Source Code
 * Copyright (C) 2016, BNC <DCFabric-admin@bnc.org.cn>
 *
 * This file is part of the BNC SDN Controller. BNC SDN
 * Controller is a free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, , see <http://www.gnu.org/licenses/>.
 */

/******************************************************************************
*                                                                             *
*   File Name   : log.h           *
*   Author      : bnc zgzhao           *
*   Create Date : 2016-5-25           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/
#ifndef __LOG_H
#define __LOG_H

#include "CLog.h"

enum LogLevel_e
{
    LOG_LEVEL_TRACE,
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_FATAL,
};

//日志宏
#ifdef LOG4CPLUS

#include "logger.h"
#include "loggingmacros.h"

extern log4cplus::Logger logger;
extern void initLogger(); 

#define LOG_TRACE(logEvent)         LOG4CPLUS_TRACE(logger, logEvent)
#define LOG_DEBUG(logEvent)         LOG4CPLUS_DEBUG(logger, logEvent)
#define LOG_INFO(logEvent)          LOG4CPLUS_INFO(logger, logEvent)
#define LOG_WARN(logEvent)          LOG4CPLUS_WARN(logger, logEvent)
#define LOG_ERROR(logEvent)         LOG4CPLUS_ERROR(logger, logEvent)
#define LOG_FATAL(logEvent)         LOG4CPLUS_FATAL(logger, logEvent)

#define LOG_TRACE_FMT(...)          LOG4CPLUS_TRACE_FMT(logger, __VA_ARGS__)
#define LOG_DEBUG_FMT(...)          LOG4CPLUS_DEBUG_FMT(logger, __VA_ARGS__)
#define LOG_INFO_FMT(...)           LOG4CPLUS_INFO_FMT(logger, __VA_ARGS__)
#define LOG_WARN_FMT(...)           LOG4CPLUS_WARN_FMT(logger, __VA_ARGS__)
#define LOG_ERROR_FMT(...)          LOG4CPLUS_ERROR_FMT(logger, __VA_ARGS__)
#define LOG_FATAL_FMT(...)          LOG4CPLUS_FATAL_FMT(logger, __VA_ARGS__)

#else

#ifdef LOG4PRINT

#define LOG_TRACE(logEvent)         printf(logEvent);printf("\n")
#define LOG_DEBUG(logEvent)         printf(logEvent);printf("\n")
#define LOG_INFO(logEvent)          printf(logEvent);printf("\n")
#define LOG_WARN(logEvent)          printf(logEvent);printf("\n")
#define LOG_ERROR(logEvent)         printf(logEvent);printf("\n")
#define LOG_FATAL(logEvent)         printf(logEvent);printf("\n")

#define LOG_TRACE_FMT(...)          printf(__VA_ARGS__);printf("\n")
#define LOG_DEBUG_FMT(...)          printf(__VA_ARGS__);printf("\n")
#define LOG_INFO_FMT(...)           printf(__VA_ARGS__);printf("\n")
#define LOG_WARN_FMT(...)           printf(__VA_ARGS__);printf("\n")
#define LOG_ERROR_FMT(...)          printf(__VA_ARGS__);printf("\n")
#define LOG_FATAL_FMT(...)          printf(__VA_ARGS__);printf("\n")

#else

const INT1* fileName(const INT1* file);

#define LOG_PROC(prefix, format, arguments...)                 \
do {                                                           \
    INT1 _b[10240];                                            \
    time_t _tm = time(NULL);                                   \
    struct tm* _ltm = localtime(&_tm);                         \
    UINT4 _l = snprintf(_b, sizeof(_b),                        \
                "[%d-%d-%d %d:%d:%d]",                         \
                _ltm->tm_year+1900,                            \
                _ltm->tm_mon+1,                                \
                _ltm->tm_mday,                                 \
                _ltm->tm_hour,                                 \
                _ltm->tm_min,                                  \
                _ltm->tm_sec);                                 \
    _l += snprintf(_b+_l, sizeof(_b)-_l, "[%s][%s:%s:%d] ",    \
          prefix, fileName(__FILE__), __FUNCTION__, __LINE__); \
    _l += snprintf(_b+_l, sizeof(_b)-_l-1, format, ##arguments); \
    if (_l < sizeof(_b)-1) {                                   \
        _l += snprintf(_b+_l, sizeof(_b)-_l, "\n");            \
        CLog::getInstance()->write(_b, _l);                    \
    } else {                                                   \
        _b[sizeof(_b)-2] = '\n';                               \
        _b[sizeof(_b)-1] = '\0';                               \
        CLog::getInstance()->write(_b, sizeof(_b));            \
    }                                                          \
} while (FALSE);

#if 0
#define LOG_TRACE(logEvent)         {if (LOG_LEVEL_TRACE >= g_logLevel) {LOG_PROC("TRACE", logEvent);printf("[TRACE] %s-%s-%d: ", fileName(__FILE__), __FUNCTION__, __LINE__);printf(logEvent);printf("\n");}}
#define LOG_DEBUG(logEvent)         {if (LOG_LEVEL_DEBUG >= g_logLevel) {LOG_PROC("DEBUG", logEvent);printf("[DEBUG] %s-%s-%d: ", fileName(__FILE__), __FUNCTION__, __LINE__);printf(logEvent);printf("\n");}}
#define LOG_INFO(logEvent)          {if (LOG_LEVEL_INFO  >= g_logLevel) {LOG_PROC("INFO",  logEvent);printf("[INFO] %s-%s-%d: ", fileName(__FILE__), __FUNCTION__, __LINE__);printf(logEvent);printf("\n");}}
#else
#define LOG_TRACE(logEvent)         {if (LOG_LEVEL_TRACE >= g_logLevel) {LOG_PROC("TRACE", logEvent);}}
#define LOG_DEBUG(logEvent)         {if (LOG_LEVEL_DEBUG >= g_logLevel) {LOG_PROC("DEBUG", logEvent);}}
#define LOG_INFO(logEvent)          {if (LOG_LEVEL_INFO  >= g_logLevel) {LOG_PROC("INFO",  logEvent);}}
#endif
#define LOG_WARN(logEvent)          {if (LOG_LEVEL_WARN  >= g_logLevel) {LOG_PROC("WARN",  logEvent);printf("[WARN] %s-%s-%d: ", fileName(__FILE__), __FUNCTION__, __LINE__);printf(logEvent);printf("\n");}}
#define LOG_ERROR(logEvent)         {if (LOG_LEVEL_ERROR >= g_logLevel) {LOG_PROC("ERROR", logEvent);printf("[ERROR] %s-%s-%d: ", fileName(__FILE__), __FUNCTION__, __LINE__);printf(logEvent);printf("\n");}}
#define LOG_FATAL(logEvent)         {if (LOG_LEVEL_FATAL >= g_logLevel) {LOG_PROC("FATAL", logEvent);printf("[FATAL] %s-%s-%d: ", fileName(__FILE__), __FUNCTION__, __LINE__);printf(logEvent);printf("\n");}}

#if 0
#define LOG_TRACE_FMT(...)          {if (LOG_LEVEL_TRACE >= g_logLevel) {LOG_PROC("TRACE", __VA_ARGS__);printf("[TRACE] %s-%s-%d: ", fileName(__FILE__), __FUNCTION__, __LINE__);printf(__VA_ARGS__);printf("\n");}}
#define LOG_DEBUG_FMT(...)          {if (LOG_LEVEL_DEBUG >= g_logLevel) {LOG_PROC("DEBUG", __VA_ARGS__);printf("[DEBUG] %s-%s-%d: ", fileName(__FILE__), __FUNCTION__, __LINE__);printf(__VA_ARGS__);printf("\n");}}
#define LOG_INFO_FMT(...)           {if (LOG_LEVEL_INFO  >= g_logLevel) {LOG_PROC("INFO",  __VA_ARGS__);printf("[INFO] %s-%s-%d: ", fileName(__FILE__), __FUNCTION__, __LINE__);printf(__VA_ARGS__);printf("\n");}}
#else
#define LOG_TRACE_FMT(...)          {if (LOG_LEVEL_TRACE >= g_logLevel) {LOG_PROC("TRACE", __VA_ARGS__);}}
#define LOG_DEBUG_FMT(...)          {if (LOG_LEVEL_DEBUG >= g_logLevel) {LOG_PROC("DEBUG", __VA_ARGS__);}}
#define LOG_INFO_FMT(...)           {if (LOG_LEVEL_INFO  >= g_logLevel) {LOG_PROC("INFO",  __VA_ARGS__);}}
#endif
#define LOG_WARN_FMT(...)           {if (LOG_LEVEL_WARN  >= g_logLevel) {LOG_PROC("WARN",  __VA_ARGS__);printf("[WARN] %s-%s-%d: ", fileName(__FILE__), __FUNCTION__, __LINE__);printf(__VA_ARGS__);printf("\n");}}
#define LOG_ERROR_FMT(...)          {if (LOG_LEVEL_ERROR >= g_logLevel) {LOG_PROC("ERROR", __VA_ARGS__);printf("[ERROR] %s-%s-%d: ", fileName(__FILE__), __FUNCTION__, __LINE__);printf(__VA_ARGS__);printf("\n");}}
#define LOG_FATAL_FMT(...)          {if (LOG_LEVEL_FATAL >= g_logLevel) {LOG_PROC("FATAL", __VA_ARGS__);printf("[FATAL] %s-%s-%d: ", fileName(__FILE__), __FUNCTION__, __LINE__);printf(__VA_ARGS__);printf("\n");}}

#endif //LOG4PRINT

#endif //LOG4CPLUS


// 宏, 用来简化, 代替的判断+输出log+return返回值
#define LOG_IF_RETURN(c, r, l) \
	if (c) \
	{ \
		LOG_INFO((l)); \
		return (r);    \
	}

// 宏, 用来简化, 代替的判断+输出log+return
#define LOG_IF_VOID(c, l) \
	if (c) \
	{ \
		LOG_INFO((l)); \
		return ;    \
	}

#endif //__LOG_H
