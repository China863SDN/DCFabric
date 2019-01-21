
#ifndef _CSWPATHLIST_H_
#define _CSWPATHLIST_H_

class CSwPathList
{
	public:
		CSwPathList();
		CSwPathList(const CSmartPtr<CSwitch >& sw);
		~CSwPathList();

		INT4 AddSwOnePath(CPathNodeList* NodeList);
		INT4 InsertSwOnePath(CPathNodeList* NodeList);
		INT4 DelSwOnePath(const CSmartPtr<CSwitch >& src_sw);
		INT4 DelSwAllPath();
		CPathNodeList* RemoveFirstPath();
		CPathNodeList* findOnePathBySw(const CSmartPtr<CSwitch >& src_sw);
		CPathNodeList* findOnePathBySw(const UINT8&  src_dpid );

		
	private:
		CSmartPtr<CSwitch >  m_sw;
		UINT4 m_num;	
		std::list<CPathNodeList*>  m_sw_path;
};
#endif
