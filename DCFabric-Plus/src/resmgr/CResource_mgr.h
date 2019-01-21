#ifndef RESOURCE_MGR_H_
#define RESOURCE_MGR_H_

#include "bnc-type.h"


class CResourceMgr
{
	public:
		 /*
	     * 默认构造函数
	     */
		CResourceMgr();
		  /*
	     * 默认析构函数
	     */
		virtual ~CResourceMgr();
		
	public:
		  /*
	     * 打印运行类
	     */
		virtual void PrintString();


		
};
#endif

