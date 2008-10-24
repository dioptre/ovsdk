#include "ovkCPluginManager.h"
#include "ovkCPluginModule.h"

#include "../../ovk_tools.h"

#include <fs/IEntryEnumerator.h>
#include <fs/Files.h>

#include <system/Memory.h>

#include <iostream>
#include <map>

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;
using namespace std;

namespace OpenViBE
{
	namespace Kernel
	{
		class CEntryEnumeratorCallBack : public FS::IEntryEnumeratorCallBack
		{
		public:

			CEntryEnumeratorCallBack(const IKernelContext& rKernelContext, vector<IPluginModule*>& rPluginModule, map<IPluginObjectDesc*, IPluginModule*>& rPluginObjectDesc)
				:m_rKernelContext(rKernelContext)
				,m_rPluginManager(rKernelContext.getPluginManager())
				,m_rPluginModule(rPluginModule)
				,m_rPluginObjectDesc(rPluginObjectDesc)
			{
			}

			virtual FS::boolean callback(
				FS::IEntryEnumerator::IEntry& rEntry,
				FS::IEntryEnumerator::IAttributes& rAttributes)
			{
				vector<IPluginModule*>::iterator i;
				for(i=m_rPluginModule.begin(); i!=m_rPluginModule.end(); i++)
				{
					CString l_sPluginModuleName;
					if(!(*i)->getFileName(l_sPluginModuleName))
					{
						return true;
					}

					if(FS::Files::equals(rEntry.getName(), (const char*)l_sPluginModuleName))
					{
						log() << LogLevel_Warning << "Module [" << CString(rEntry.getName()) << "] has already been loaded\n";
						return true;
					}
				}

				IPluginModule* l_pPluginModule=new CPluginModule(m_rKernelContext);
				CString l_sLoadError;
				if(!l_pPluginModule->load(rEntry.getName(), &l_sLoadError))
				{
					delete l_pPluginModule;
					log() << LogLevel_Warning << "File [" << CString(rEntry.getName()) << "] is not a plugin module (error:" << l_sLoadError << ")\n";
					return true;
				}

				if(!l_pPluginModule->initialize())
				{
					l_pPluginModule->uninitialize();
					l_pPluginModule->unload();
					delete l_pPluginModule;
					log() << LogLevel_Warning << "Module [" << CString(rEntry.getName()) << "] did not initialize correctly\n";
					return true;
				}

				boolean l_bPluginObjectDescAdded=false;
				uint32 l_ui32Index=0;
				uint32 l_ui32Count=0;
				IPluginObjectDesc* l_pPluginObjectDesc=NULL;
				while(l_pPluginModule->getPluginObjectDescription(l_ui32Index, l_pPluginObjectDesc))
				{
					boolean l_bFound=false;

					map<IPluginObjectDesc*, IPluginModule*>::const_iterator i;
					for(i=m_rPluginObjectDesc.begin(); i!=m_rPluginObjectDesc.end() && !l_bFound; i++)
					{
						if(i->first->getClassIdentifier()==l_pPluginObjectDesc->getClassIdentifier())
						{
							log() << LogLevel_ImportantWarning << "Duplicate plugin object descriptor class identifier [" << i->first->getName() << "] and [" << l_pPluginObjectDesc->getName() << "]... second one is ignored.\n";
							l_bFound=true;
						}
					}

					if(!l_bFound)
					{
						if(!l_bPluginObjectDescAdded)
						{
							m_rPluginModule.push_back(l_pPluginModule);
							l_bPluginObjectDescAdded=true;
						}
						m_rPluginObjectDesc[l_pPluginObjectDesc]=l_pPluginModule;
						l_ui32Count++;
					}
					l_ui32Index++;
					l_pPluginObjectDesc=NULL;
				}

				if(l_bPluginObjectDescAdded)
				{
					log() << LogLevel_Info << "Added " << l_ui32Count << " plugin object descriptor(s) from [" << CString(rEntry.getName()) << "]\n";
				}
				else
				{
					log() << LogLevel_Warning << "No 'plugin object descriptor' found from [" << CString(rEntry.getName()) << "] even if it looked like a plugin module\n";
				}

				return true;
			}

		protected:

			ILogManager& log(void)
			{
				return m_rKernelContext.getLogManager();
			}

		protected:

			const IKernelContext& m_rKernelContext;
			IPluginManager& m_rPluginManager;
			vector<IPluginModule*>& m_rPluginModule;
			map<IPluginObjectDesc*, IPluginModule*>& m_rPluginObjectDesc;
		};
	};
};

CPluginManager::CPluginManager(const IKernelContext& rKernelContext)
	:TKernelObject<IPluginManager>(rKernelContext)
{
}

CPluginManager::~CPluginManager(void)
{
	map < IPluginObjectDesc*, vector < IPluginObject* > >::iterator i;
	vector < IPluginObject* >::iterator j;
	for(i=m_vPluginObject.begin(); i!=m_vPluginObject.end(); i++)
	{
		for(j=i->second.begin(); j!=i->second.end(); j++)
		{
			log() << LogLevel_ImportantWarning << "Trying to release plugin object with class id " << (*j)->getClassIdentifier() << " and plugin object descriptor " << i->first->getName() << " at plugin manager destruction time\n";
			(*j)->release();
		}
	}
}

boolean CPluginManager::addPluginsFromFiles(
	const CString& rFileNameWildCard)
{
	log() << LogLevel_Info << "Adding [" << rFileNameWildCard << "]\n";

	boolean l_bResult;
	CEntryEnumeratorCallBack l_rCB(getKernelContext(), m_vPluginModule, m_vPluginObjectDesc);
	FS::IEntryEnumerator* l_pEntryEnumerator=FS::createEntryEnumerator(l_rCB);
	l_bResult=l_pEntryEnumerator->enumerate(rFileNameWildCard);
	l_pEntryEnumerator->release();
	return l_bResult;
}

CIdentifier CPluginManager::getNextPluginObjectDescIdentifier(
	const CIdentifier& rPreviousIdentifier) const
{
	boolean l_bFoundPrevious=(rPreviousIdentifier==OV_UndefinedIdentifier);
	map < IPluginObjectDesc*, IPluginModule* >::const_iterator i;
	for(i=m_vPluginObjectDesc.begin(); i!=m_vPluginObjectDesc.end(); i++)
	{
		if(!l_bFoundPrevious)
		{
			if(i->first->getClassIdentifier()==rPreviousIdentifier)
			{
				l_bFoundPrevious=true;
			}
		}
		else
		{
			return i->first->getClassIdentifier();
		}
	}
	return OV_UndefinedIdentifier;
}

CIdentifier CPluginManager::getNextPluginObjectDescIdentifier(
	const CIdentifier& rPreviousIdentifier,
	const CIdentifier& rBaseClassIdentifier) const
{
	boolean l_bFoundPrevious=(rPreviousIdentifier==OV_UndefinedIdentifier);
	map < IPluginObjectDesc*, IPluginModule* >::const_iterator i;
	for(i=m_vPluginObjectDesc.begin(); i!=m_vPluginObjectDesc.end(); i++)
	{
		if(!l_bFoundPrevious)
		{
			if(i->first->getClassIdentifier()==rPreviousIdentifier)
			{
				l_bFoundPrevious=true;
			}
		}
		else
		{
			if(i->first->isDerivedFromClass(rBaseClassIdentifier))
			{
				return i->first->getClassIdentifier();
			}
		}
	}
	return OV_UndefinedIdentifier;
}

boolean CPluginManager::canCreatePluginObject(
	const CIdentifier& rClassIdentifier)
{
	log() << LogLevel_Debug << "Searching if can build plugin object\n";

	map < IPluginObjectDesc*, IPluginModule* >::const_iterator i;
	for(i=m_vPluginObjectDesc.begin(); i!=m_vPluginObjectDesc.end(); i++)
	{
		if(i->first->getCreatedClass()==rClassIdentifier)
		{
			return true;
		}
	}

	return false;
}

const IPluginObjectDesc* CPluginManager::getPluginObjectDesc(
	const CIdentifier& rClassIdentifier) const
{
	log() << LogLevel_Debug << "Searching plugin object descriptor\n";

	map < IPluginObjectDesc*, IPluginModule* >::const_iterator i;
	for(i=m_vPluginObjectDesc.begin(); i!=m_vPluginObjectDesc.end(); i++)
	{
		if(i->first->getClassIdentifier()==rClassIdentifier)
		{
			return i->first;
		}
	}

	log() << LogLevel_Warning << "Plugin object descriptor class identifier " << rClassIdentifier << " not found\n";
	return NULL;
}

const IPluginObjectDesc* CPluginManager::getPluginObjectDescCreating(
	const CIdentifier& rClassIdentifier) const
{
	log() << LogLevel_Debug << "Searching plugin object descriptor\n";

	map < IPluginObjectDesc*, IPluginModule* >::const_iterator i;
	for(i=m_vPluginObjectDesc.begin(); i!=m_vPluginObjectDesc.end(); i++)
	{
		if(i->first->getCreatedClass()==rClassIdentifier)
		{
			return i->first;
		}
	}

	log() << LogLevel_Warning << "Plugin object descriptor class identifier " << rClassIdentifier << " not found\n";
	return NULL;

}

namespace
{
	struct SBoxProto : public IBoxProto
	{
	public:

		SBoxProto(void)
			:m_bIsDeprecated(false)
			,m_bIsUnstable(false)
		{
		}
		virtual uint32 addInput(const CString& sName, const CIdentifier& rTypeIdentifier)
		{
			uint64 v=rTypeIdentifier.toUInteger();
			swap_byte(v, 5, 1);
			swap_byte(v, 4, 2);
			swap_byte(v, 3, 0);
			m_oHash=m_oHash.toUInteger()^v;
			return true;
		}
		virtual uint32 addOutput(const CString& sName, const CIdentifier& rTypeIdentifier)
		{
			uint64 v=rTypeIdentifier.toUInteger();
			swap_byte(v, 7, 5);
			swap_byte(v, 6, 0);
			swap_byte(v, 2, 1);
			m_oHash=m_oHash.toUInteger()^v;
			return true;
		}
		virtual uint32 addSetting(const CString& sName, const CIdentifier& rTypeIdentifier, const CString& sDefaultValue)
		{
			uint64 v=rTypeIdentifier.toUInteger();
			swap_byte(v, 6, 4);
			swap_byte(v, 5, 0);
			swap_byte(v, 3, 1);
			m_oHash=m_oHash.toUInteger()^v;
			return true;
		}
		virtual boolean addFlag(const EBoxFlag eBoxFlag)
		{
			switch(eBoxFlag)
			{
				case BoxFlag_CanAddInput:       m_oHash=m_oHash.toUInteger()^OpenViBE::CIdentifier(0x07507AC8, 0xEB643ACE).toUInteger(); break;
				case BoxFlag_CanModifyInput:    m_oHash=m_oHash.toUInteger()^OpenViBE::CIdentifier(0x5C985376, 0x8D74CDB8).toUInteger(); break;
				case BoxFlag_CanAddOutput:      m_oHash=m_oHash.toUInteger()^OpenViBE::CIdentifier(0x58DEA69B, 0x12411365).toUInteger(); break;
				case BoxFlag_CanModifyOutput:   m_oHash=m_oHash.toUInteger()^OpenViBE::CIdentifier(0x6E162C01, 0xAC979F22).toUInteger(); break;
				case BoxFlag_CanAddSetting:     m_oHash=m_oHash.toUInteger()^OpenViBE::CIdentifier(0xFA7A50DC, 0x2140C013).toUInteger(); break;
				case BoxFlag_CanModifySetting:  m_oHash=m_oHash.toUInteger()^OpenViBE::CIdentifier(0x624D7661, 0xD8DDEA0A).toUInteger(); break;
				case BoxFlag_IsDeprecated:      m_bIsDeprecated=true; break;
				case BoxFlag_IsUnstable:        m_bIsUnstable=true;   break;
				default:
					return false;
					break;
			}
			return true;
		}
		void swap_byte(uint64& v, uint32 i, uint32 j)
		{
			uint8 s, V[sizeof(v)];
			System::Memory::hostToLittleEndian(v, V);
			s=V[i];
			V[i]=V[j];
			V[j]=s;
			System::Memory::littleEndianToHost(V, &v);
		}

		_IsDerivedFromClass_Final_(IBoxProto, OV_UndefinedIdentifier)

		CIdentifier m_oHash;
		boolean m_bIsDeprecated;
		boolean m_bIsUnstable;
	};
}

CIdentifier CPluginManager::getPluginObjectHashValue(
	const CIdentifier& rClassIdentifier) const
{
	const IPluginObjectDesc* l_pPluginObjectDesc=this->getPluginObjectDescCreating(rClassIdentifier);
	const IBoxAlgorithmDesc* l_pBoxAlgorithmDesc=dynamic_cast<const IBoxAlgorithmDesc*>(l_pPluginObjectDesc);
	if(l_pBoxAlgorithmDesc)
	{
		SBoxProto l_oBoxPrototype;
		l_pBoxAlgorithmDesc->getBoxPrototype(l_oBoxPrototype);
		return l_oBoxPrototype.m_oHash;
	}
	return OV_UndefinedIdentifier;
}

boolean CPluginManager::isPluginObjectFlaggedAsDeprecated(
	const CIdentifier& rClassIdentifier) const
{
	const IPluginObjectDesc* l_pPluginObjectDesc=this->getPluginObjectDescCreating(rClassIdentifier);
	const IBoxAlgorithmDesc* l_pBoxAlgorithmDesc=dynamic_cast<const IBoxAlgorithmDesc*>(l_pPluginObjectDesc);
	if(l_pBoxAlgorithmDesc)
	{
		SBoxProto l_oBoxPrototype;
		l_pBoxAlgorithmDesc->getBoxPrototype(l_oBoxPrototype);
		return l_oBoxPrototype.m_bIsDeprecated;
	}
	return false;
}

boolean CPluginManager::isPluginObjectFlaggedAsUnstable(
	const CIdentifier& rClassIdentifier) const
{
	const IPluginObjectDesc* l_pPluginObjectDesc=this->getPluginObjectDescCreating(rClassIdentifier);
	const IBoxAlgorithmDesc* l_pBoxAlgorithmDesc=dynamic_cast<const IBoxAlgorithmDesc*>(l_pPluginObjectDesc);
	if(l_pBoxAlgorithmDesc)
	{
		SBoxProto l_oBoxPrototype;
		l_pBoxAlgorithmDesc->getBoxPrototype(l_oBoxPrototype);
		return l_oBoxPrototype.m_bIsUnstable;
	}
	return false;
}

IPluginObject* CPluginManager::createPluginObject(
	const CIdentifier& rClassIdentifier)
{
	return createPluginObjectT<IPluginObject, IPluginObjectDesc>(rClassIdentifier, NULL);
}

boolean CPluginManager::releasePluginObject(
	IPluginObject* pPluginObject)
{
	log() << LogLevel_Debug << "Releasing plugin object\n";

	map < IPluginObjectDesc*, vector < IPluginObject* > >::iterator i;
	vector < IPluginObject* >::iterator j;
	for(i=m_vPluginObject.begin(); i!=m_vPluginObject.end(); i++)
	{
		for(j=i->second.begin(); j!=i->second.end(); j++)
		{
			if((*j)==pPluginObject)
			{
				i->second.erase(j);
				pPluginObject->release();
				return true;
			}
		}
	}

	log() << LogLevel_Warning << "Plugin object has not been created by this plugin manager (class id was " << pPluginObject->getClassIdentifier() << ")\n";
	return false;
}


IAlgorithm* CPluginManager::createAlgorithm(
	const CIdentifier& rClassIdentifier,
	const IAlgorithmDesc** ppAlgorithmDesc)
{
	return createPluginObjectT<IAlgorithm, IAlgorithmDesc>(rClassIdentifier, ppAlgorithmDesc);
}

IBoxAlgorithm* CPluginManager::createBoxAlgorithm(
	const CIdentifier& rClassIdentifier,
	const IBoxAlgorithmDesc** ppBoxAlgorithmDesc)
{
	return createPluginObjectT<IBoxAlgorithm, IBoxAlgorithmDesc>(rClassIdentifier, ppBoxAlgorithmDesc);
}

template <class IPluginObjectT, class IPluginObjectDescT>
IPluginObjectT* CPluginManager::createPluginObjectT(
	const CIdentifier& rClassIdentifier,
	const IPluginObjectDescT** ppPluginObjectDescT)
{
	if(ppPluginObjectDescT)
	{
		*ppPluginObjectDescT=NULL;
	}

	IPluginObjectDesc* l_pPluginObjectDesc=NULL;
	map < IPluginObjectDesc*, IPluginModule* >::const_iterator i;
	for(i=m_vPluginObjectDesc.begin(); i!=m_vPluginObjectDesc.end(); i++)
	{
		if(i->first->getCreatedClass()==rClassIdentifier)
		{
			l_pPluginObjectDesc=i->first;
		}
	}
	if(!l_pPluginObjectDesc)
	{
		return NULL;
	}

	IPluginObject* l_pPluginObject=l_pPluginObjectDesc->create();
	if(!l_pPluginObjectDesc)
	{
		return NULL;
	}

	IPluginObjectDescT* l_pPluginObjectDescT=dynamic_cast<IPluginObjectDescT*>(l_pPluginObjectDesc);
	IPluginObjectT* l_pPluginObjectT=dynamic_cast<IPluginObjectT*>(l_pPluginObject);
	if(!l_pPluginObjectDescT || !l_pPluginObjectT)
	{
		l_pPluginObject->release();
		return NULL;
	}

	if(ppPluginObjectDescT)
	{
		*ppPluginObjectDescT=l_pPluginObjectDescT;
	}

	m_vPluginObject[l_pPluginObjectDescT].push_back(l_pPluginObjectT);
	return l_pPluginObjectT;
}
