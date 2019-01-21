
#ifndef PROXY_RES_H_
#define  PROXY_RES_H_

#include "bnc-type.h"
#include "CResource_mgr.h"

class 	CProxyResMgr: public CResourceMgr
{
	
	private: 
		/*
		��ȡ���캯��
		*/
		CProxyResMgr();
		
	public:
		/*
		Ĭ����������
		*/
		~CProxyResMgr();
		CResourceMgr* getResource(CResourceMgr* resource);
		/*
		��ӡ������
		*/
		void PrintString();

		/*
		��ʼ������
		*/
		INT4 init();
		/*
		��ȡ����ʵ��
		*/
		static CProxyResMgr* getInstance();
		
		
	private:
		CResourceMgr* m_pResource;
		static CProxyResMgr* m_pInstance;

	
};

#endif
