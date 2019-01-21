
#ifndef PROXY_RES_H_
#define  PROXY_RES_H_

#include "bnc-type.h"
#include "CResource_mgr.h"

class 	CProxyResMgr: public CResourceMgr
{
	
	private: 
		/*
		获取构造函数
		*/
		CProxyResMgr();
		
	public:
		/*
		默认析构函数
		*/
		~CProxyResMgr();
		CResourceMgr* getResource(CResourceMgr* resource);
		/*
		打印运行类
		*/
		void PrintString();

		/*
		初始化代理
		*/
		INT4 init();
		/*
		获取运行实例
		*/
		static CProxyResMgr* getInstance();
		
		
	private:
		CResourceMgr* m_pResource;
		static CProxyResMgr* m_pInstance;

	
};

#endif
