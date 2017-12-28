/*
 * GNFlush SDN Controller GPL Source Code
 * Copyright (C) 2015, Greenet <greenet@greenet.net.cn>
 *
 * This file is part of the GNFlush SDN Controller. GNFlush SDN
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
 *   File Name   : log.c           *
 *   Author      : greenet Administrator           *
 *   Create Date : 2015-3-3           *
 *   Version     : 1.0           *
 *   Function    : .           *
 *                                                                             *
 ******************************************************************************/

#include "common.h"
#include "timer.h"

#define LOG_FILE_SIZE   1024*1024*100//200  //200M
#define LOG_WRITE_SIZE  1024*50        //50K
#define LOG_BUFFER_SIZE 1024*1024      //1M

static p_loop_buffer pLogBuf = NULL;
static pthread_t logThread = 0;

void write_log(char* str, unsigned int len)
{

    if (pLogBuf) {
        buffer_write(pLogBuf, str, len);
    }

}

static char* create_file(char* file, UINT4 size)
{
    struct timeval curTime;
    gettimeofday(&curTime, NULL);
    struct tm* localTime = localtime((time_t*)(&curTime));

    snprintf(file, size, 
        "/var/log/DCFabric_%04d%02d%02d%02d%02d%02d.log", 
        localTime->tm_year + 1900,
        localTime->tm_mon + 1,
        localTime->tm_mday,    
        localTime->tm_hour, 
        localTime->tm_min, 
        localTime->tm_sec);  
    return file;
}

static FILE* open_file(char* file)
{
    FILE* fp = fopen(file, "w+");
    if (!fp) {
        printf("open file %s failed[%d]!\n", file, errno);
    }
    return fp;
}

static void write_file(char* str, UINT4 len)
{
    static char file[100] = {0};
    static FILE* fp = NULL;

    if (!fp) {
        fp = open_file(create_file(file, sizeof(file)));
    }

    if (fp) {
        while (len > 0) {
            printf("------------------------ write %d bytes into file %s .............................\n", len, file);
            size_t write = fwrite(str, sizeof(char), len, fp);
            fflush(fp);
            if (write <= 0) {
                printf("write file %s failed[%d]!\n", file, errno);
                fclose(fp);
                fp = NULL;
                return;
            } else {
                if (write >= len)
                    break;
                len -= write;
                str += write;
            }
        }

        if (ftell(fp) >= LOG_FILE_SIZE) {
            printf("------------------------ size of file %s is %d, reaching %d bytes .............................\n", file, ftell(fp), LOG_FILE_SIZE);
            fclose(fp);
            fp = NULL;
        }
    }
}

static void *log_task()
{
    static char buffer[LOG_BUFFER_SIZE] = {0};

    while (1) {
        if (pLogBuf && (pLogBuf->cur_len >= LOG_WRITE_SIZE)) {
            UINT4 len = pLogBuf->cur_len;
            buffer_read(pLogBuf, buffer, len, FALSE);
            write_file(buffer, len);
        }
        MsecSleep(50);
    }

    return NULL;
}

void init_log_task()
{
    if (!(pLogBuf = init_loop_buffer(LOG_BUFFER_SIZE))) {
        printf("malloc log buffer failed!\n");
        return;
    }

    if (pthread_create(&logThread, NULL, log_task, NULL) != 0) {
        printf("pthread_create logThread failed[%d]!\n", errno);
    }
}
