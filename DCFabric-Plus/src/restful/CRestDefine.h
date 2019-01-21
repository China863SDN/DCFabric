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
*   File Name   : CRestDefine.h      *
*   Author      : bnc xflu           *
*   Create Date : 2016-7-21          *
*   Version     : 1.0                *
*   Function    : .                  *
*                                                                             *
******************************************************************************/
#ifndef _CRESTDEFINE_H
#define _CRESTDEFINE_H

#include <string>
#include <map>

/*
 * 存储了一些http协议头部所需要的enmu值
 */
namespace bnc
{
	namespace restful
	{
		enum http_method
		{
			HTTP_GET = 0,
			HTTP_POST,
			HTTP_PUT,
			HTTP_DELETE,
			HTTP_ANY,			///< 自定义, 用来标识任意种类
			HTTP_METHOD_OTHER
		};

		enum http_version
		{
			HTTP_1_0 = 0,
			HTTP_1_1,
			HTTP_VERSION_OHTER
		};

		enum http_status
		{
			STATUS_OK = 200,
			STATUS_BAD_REQUEST = 400,
			STATUS_UNAUTHORIZED = 401,
			STATUS_FORBIDDEN = 403,
			STATUS_NOT_FOUND = 404,
			STATUS_OTHER,
		};

		enum content_type
		{
		    CONTENT_JSON = 0
		};
	}
}

typedef std::map<bnc::restful::http_method, std::string> method_map;
typedef std::map<bnc::restful::http_version, std::string> version_map;
typedef std::map<bnc::restful::http_status, std::string> status_map;
typedef std::map<bnc::restful::content_type, std::string> content_type_map;

/*
 * 定义了HTTP所需要的enum以及相应的处理函数
 * 为方便使用, 使用单例模式未提供静态方法
 * 会在CRestApi初始化的时候进行初始化
 */
class CRestDefine
{
public:
	/*
	 * 默认析构函数
	 */
	~CRestDefine();
	/*
	 * 获取实例
	 */
	static CRestDefine* getInstance();

	/*
	 * 从字符串中取得method
	 */
	bnc::restful::http_method getMethodFromStr(const std::string & raw);

	/*
	 * 从字符串中取得version
	 */
	bnc::restful::http_version getVersionFromStr(const std::string & raw);

	/*
	 * 从字符串中取得status
	 */
	bnc::restful::http_status getStatusFromStr(const std::string & raw);

    /*
     * 将method转成字符串
     */
    void getStrFromMethod(bnc::restful::http_method method, std::string & str_method);

	/*
	 * 将version转成字符串
	 */
	void getStrFromVersion(bnc::restful::http_version version, std::string & str_version);

	/*
	 * 将status转成字符串
	 */
	void getStrFromStatus(bnc::restful::http_status status, std::string & str_status);

	/*
	 * 将content_type转成字符串
	 */
	void getStrFromContentType(bnc::restful::content_type type, std::string & str_type);

	/*
	 * 获取已经注册的method list
	 */
	method_map getMethodList() { return m_methodList; }

private:
	/*
	 * 默认构造函数
	 */
	CRestDefine();

	/*
	 * 初始化
	 */
	void init();

	static CRestDefine* m_pInstance;		///< 静态实例
	method_map m_methodList;				///< method list
	version_map m_versionList;				///< version list
	status_map m_statusList;				///< status list
	content_type_map m_contenttypeList;     ///< content type list
};

#endif /* SRC_RESTFUL_CRESTDEFINE_H_ */
