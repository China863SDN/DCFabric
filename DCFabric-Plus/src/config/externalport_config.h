
#ifndef _EXTERNALPORT_CONFIG_H_
#define _EXTERNALPRT_CONFIG_H_

class externalport_config
{
	private:
		externalport_config();
	public:
		~externalport_config();
		
		INT4  read_externalport_config();
		static externalport_config* getInstance();

	private:
		static externalport_config* m_pInstance;

};

#endif
