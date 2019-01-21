/*
 * BNC SDN Controller GPL Source Code
 * Copyright (C) 2015, BNC <DCFabric-admin@bnc.org.cn>
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
 *   File Name   : CLog.cpp                                                    *
 *   Author      : bnc bojiang                                                 *
 *   Create Date : 2017-10-30                                                  *
 *   Version     : 1.0                                                         *
 *   Function    :                                                             *
 *                                                                             *
 ******************************************************************************/
#include <sys/prctl.h> 
#include "CLog.h"
#include "log.h"
#include "bnc-error.h"
#include "CConf.h"

#define LOG_FILE_SIZE   1024*1024*200       //200M
#define LOG_BUFFER_SIZE 1024*1024           //1M
#define LOG_WRITE_SIZE  LOG_BUFFER_SIZE/10  //100K

INT4 g_logLevel = LOG_LEVEL_TRACE;
static std::string logDir = "/var/log/";
static UINT4 logFileSize = LOG_FILE_SIZE;

static void *runLog(void* param)
{
    if (NULL == param)
    {
        LOG_ERROR("runLog thread start failed, null param!");
        return NULL;
    }

	prctl(PR_SET_NAME, (unsigned long)"LogRun");  

    CLog* clog = (CLog*)param;
    INT4 pid = getpid();
    UINT8 threadId = (UINT8)pthread_self();

    LOG_DEBUG_FMT("CLog start runLog thread with pid[%d]threadId[%llu] ...", pid, threadId);

    while (1)
    {
        try
        {
            if (clog->waitWriteFile() == 0)
            {
                clog->writeFile();
            }
            else
            {
                LOG_ERROR_FMT("CLog runLog thread sem_wait failed[%d]!", errno);
            }
        }
        catch (...)
        {
            LOG_ERROR("catch exception !");
        }
    }

    LOG_DEBUG_FMT("CLog runLog thread stop, threadId:%llu", threadId);

    return NULL;
}

CLog* CLog::m_instance = NULL;

CLog* CLog::getInstance()
{
    if (NULL == m_instance) 
    {
        m_instance = new CLog();
        if (NULL == m_instance)
        {
            exit(-1);
        }
    }

    return (m_instance);
}

CLog::CLog():m_buffer(LOG_BUFFER_SIZE, TRUE)
{
}

CLog::~CLog()
{
}

INT4 CLog::init()
{
    const INT1* pDir = CConf::getInstance()->getConfig("log_conf", "log_dir");
    if (NULL != pDir)
    {
        INT1 dir[100] = {0};
        strncpy(dir, pDir, 99);
        dir[99] = 0;

        UINT4 length = strlen(dir);
        if ('/' != dir[length-1])
        {
            if (99 > length)
            {
                dir[length] = '/';
                dir[length+1] = 0;
            }
            else
            {
                dir[length-1] = '/';
                dir[length] = 0;
            }
        }

        if (access(dir, 0) == 0)
        {
            logDir = dir;
        }
        else
        {
            LOG_WARN_FMT("configured log_dir[%s] unexist, use default /var/log/ !", dir);
        }
    }
    LOG_INFO_FMT("log_dir %s", logDir.c_str());

    const INT1* pLevel = CConf::getInstance()->getConfig("log_conf", "log_level");
    if (NULL != pLevel)
    {
        (strcasecmp(pLevel, "TRACE") == 0) ? (g_logLevel = LOG_LEVEL_TRACE) :
        (strcasecmp(pLevel, "DEBUG") == 0) ? (g_logLevel = LOG_LEVEL_DEBUG) :
        (strcasecmp(pLevel, "INFO") == 0) ? (g_logLevel = LOG_LEVEL_INFO) :
        (strcasecmp(pLevel, "WARN") == 0) ? (g_logLevel = LOG_LEVEL_WARN) :
        (strcasecmp(pLevel, "ERROR") == 0) ? (g_logLevel = LOG_LEVEL_ERROR) :
        (strcasecmp(pLevel, "FATAL") == 0) ? (g_logLevel = LOG_LEVEL_FATAL) :
        (g_logLevel = LOG_LEVEL_TRACE);
    }
    LOG_INFO_FMT("log_level %d", g_logLevel);

    const INT1* pSize = CConf::getInstance()->getConfig("log_conf", "log_file_size");
    logFileSize = (NULL == pSize) ? LOG_FILE_SIZE : (atoll(pSize)*1024*1024);
    LOG_INFO_FMT("log_file_size %u", logFileSize);

    return m_thread.init(runLog, this, "CLog");
}

INT4 CLog::write(const INT1* data, UINT4 length)
{
    if (m_buffer.write(data, length))
    {
        if (m_buffer.length() >= LOG_WRITE_SIZE)
        {
            m_sem.post();
        }
    }

    return BNC_OK;
}

INT4 CLog::waitWriteFile()
{
    return m_sem.wait();
}

std::string CLog::createFile()
{
    struct timeval curTime;
    gettimeofday(&curTime, NULL);
    struct tm* localTime = localtime((time_t*)(&curTime));

    char file[100] = {0};
    snprintf(file, 100, 
        "DCFabric_%04d%02d%02d%02d%02d%02d.log", 
        localTime->tm_year + 1900,
        localTime->tm_mon + 1,
        localTime->tm_mday,    
        localTime->tm_hour, 
        localTime->tm_min, 
        localTime->tm_sec);  

    return std::string(file);
}

void CLog::writeFile()
{
    static FILE* fp = NULL;

    if (NULL == fp) 
    {
        m_file = logDir + createFile();
        fp = fopen(m_file.c_str(), "w+");
        if (NULL == fp) 
        {
            LOG_ERROR_FMT("CLog open file[%s] failed[%d] !", m_file.c_str(), errno);
        }
    }

    if (NULL != fp) 
    {
        static char buffer[LOG_BUFFER_SIZE] = {0};
        char* str = buffer;
        UINT4 length = m_buffer.length();

        if (!(m_buffer.read(buffer, length)))
            return;

        while (length > 0) 
        {
            //LOG_DEBUG_FMT("-------- write %u bytes into file %s ........\n", len, m_file.c_str());
            size_t write = fwrite(str, sizeof(char), length, fp);
            fflush(fp);
            if (write <= 0) 
            {
                printf("write file %s failed[%d]!\n", m_file.c_str(), errno);
                fclose(fp);
                fp = NULL;
                return;
            } 
            else 
            {
                if (write >= length)
                    break;
                length -= write;
                str += write;
            }
        }

        if (ftell(fp) >= logFileSize) 
        {
            //LOG_DEBUG_FMT("-------- size of file %s is %d, reaching %d bytes ........\n", m_file.c_str(), ftell(fp), logFileSize);
            fclose(fp);
            fp = NULL;
        }
    }
}

