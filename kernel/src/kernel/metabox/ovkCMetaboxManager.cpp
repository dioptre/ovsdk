#include "ovkCMetaboxManager.h"

#include "../../ovk_tools.h"

#include <fs/IEntryEnumerator.h>

#include <map>
#include <algorithm>
#include <random>

#include "../../tools/ovkSBoxProto.h"
#include "ovp_global_defines.h"
#include "ovkCMetaboxObjectDesc.h"

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Metabox;
using namespace std;

namespace OpenViBE
{
	namespace Kernel
	{
		class CMetaboxManagerEntryEnumeratorCallBack : public TKernelObject < IObject >, public FS::IEntryEnumeratorCallBack
		{
		public:

			CMetaboxManagerEntryEnumeratorCallBack(const OpenViBE::Kernel::IKernelContext& kernelContext, CMetaboxManager& metaboxManager)
				: TKernelObject < IObject >(kernelContext), m_MetaboxManager(metaboxManager)
			{
				m_MetaBoxCount = 0;
			}

			bool callback(FS::IEntryEnumerator::IEntry& rEntry, FS::IEntryEnumerator::IAttributes& rAttributes)
			{
				if(rAttributes.isFile())
				{
					const char* fullFileName = rEntry.getName();

					OpenViBE::CIdentifier scenarioId, metaboxId, metaboxHash;
					this->getKernelContext().getScenarioManager().importScenarioFromFile(scenarioId, OV_ScenarioImportContext_OnLoadMetaboxImport, fullFileName);
					if (scenarioId != OV_UndefinedIdentifier)
					{
						OpenViBE::Kernel::IScenario& metaboxScenario = this->getKernelContext().getScenarioManager().getScenario(scenarioId);
						bool isValid = metaboxId.fromString(metaboxScenario.getAttributeValue(OVP_AttributeId_Metabox_Identifier));
						if (isValid && metaboxScenario.getAttributeValue(OV_AttributeId_Scenario_Name) != CString())
						{
							bool hasHash = metaboxHash.fromString(metaboxScenario.getAttributeValue(OV_AttributeId_Scenario_MetaboxHash));
							if (!hasHash)
							{
								this->getKernelContext().getLogManager() << LogLevel_Warning << "The metabox " << metaboxId.toString().toASCIIString() << " has no Hash in the scenario " << fullFileName << "\n";
							}
							m_MetaboxManager.setMetaboxFilePath(metaboxId, CString(fullFileName));
							m_MetaboxManager.setMetaboxHash(metaboxId, metaboxHash);
							m_MetaboxManager.setMetaboxObjectDesc(metaboxId, new CMetaboxObjectDesc(metaboxId.toString().toASCIIString(), metaboxScenario));
							m_MetaBoxCount++;
						}
						else
						{
							this->getKernelContext().getLogManager() << LogLevel_Warning << "The metabox file " << fullFileName << " is missing elements. Please check it.\n";
						}
					}
					this->getKernelContext().getScenarioManager().releaseScenario(scenarioId);
				}
				return true;
			}

			uint32_t resetMetaboxCount(void)
			{
				uint32_t returnValue = m_MetaBoxCount;
				m_MetaBoxCount = 0;
				return returnValue;
			}

			_IsDerivedFromClass_Final_(TKernelObject < IObject >, OV_UndefinedIdentifier)
		protected:
			CMetaboxManager& m_MetaboxManager;
			uint32_t m_MetaBoxCount;

		};
	}
}

CMetaboxManager::CMetaboxManager(const IKernelContext& kernelContext)
	: TKernelObject<IMetaboxManager>(kernelContext)
{
	this->getScenarioManager().registerScenarioImporter(OV_ScenarioImportContext_OnLoadMetaboxImport, ".mxb", OVP_GD_ClassId_Algorithm_XMLScenarioImporter);
}

CMetaboxManager::~CMetaboxManager(void)
{
	for (auto desc : m_MetaboxObjectDesc)
	{
		delete desc.second;
	}
}

bool CMetaboxManager::addMetaboxesFromFiles(const CString& fileNameWildCard)
{
	this->getLogManager() << LogLevel_Info << "Adding metaboxes from [" << fileNameWildCard << "]\n";

	CMetaboxManagerEntryEnumeratorCallBack l_rCallback(this->getKernelContext(), *this); //, m_vPluginModule, m_vPluginObjectDesc, haveAllPluginsLoadedCorrectly);
	FS::IEntryEnumerator* entryEnumerator = FS::createEntryEnumerator(l_rCallback);
	stringstream ss(fileNameWildCard.toASCIIString());
	string path;
	while(getline(ss, path, ';'))
	{
		bool result = false; // Used to output imported metabox count
		CString ext("");
		while((ext = this->getScenarioManager().getNextScenarioImporter(OV_ScenarioImportContext_OnLoadMetaboxImport, ext)) != CString(""))
		{
			result |= entryEnumerator->enumerate((path + "*" + ext.toASCIIString()).c_str());
		}
		if(result)
		{
			this->getLogManager() << LogLevel_Info << "Added " << l_rCallback.resetMetaboxCount() << " metaboxes from [" << path.c_str() << "]\n";
		}
	}
	entryEnumerator->release();

	return true;
}

CIdentifier CMetaboxManager::getNextMetaboxObjectDescIdentifier(const CIdentifier& previousIdentifier) const
{
	if (m_MetaboxObjectDesc.empty())
	{
		return OV_UndefinedIdentifier;
	}
	if (previousIdentifier == OV_UndefinedIdentifier)
	{
		return m_MetaboxObjectDesc.begin()->first;
	}

	auto result = m_MetaboxObjectDesc.find(previousIdentifier);
	if (result == m_MetaboxObjectDesc.end() || std::next(result, 1) == m_MetaboxObjectDesc.end())
	{
		return OV_UndefinedIdentifier;
	}
	return std::next(result, 1)->first;
}

const OpenViBE::Plugins::IPluginObjectDesc* CMetaboxManager::getMetaboxObjectDesc(const CIdentifier& metaboxIdentifier) const
{
	auto result = m_MetaboxObjectDesc.find(metaboxIdentifier);
	return result != m_MetaboxObjectDesc.end() ? result->second : nullptr;
}

void CMetaboxManager::setMetaboxObjectDesc(const OpenViBE::CIdentifier& metaboxIdentifier, OpenViBE::Plugins::IPluginObjectDesc* metaboxDescriptor)
{
	m_MetaboxObjectDesc[metaboxIdentifier] = metaboxDescriptor;
}

OpenViBE::CString CMetaboxManager::getMetaboxFilePath(const OpenViBE::CIdentifier& metaboxIdentifier) const
{
	auto resultIt = m_MetaboxFilePath.find(metaboxIdentifier);
	return resultIt != m_MetaboxFilePath.end() ? resultIt->second : OpenViBE::CString();
}

void CMetaboxManager::setMetaboxFilePath(const OpenViBE::CIdentifier& metaboxIdentifier, const CString &filePath)
{
	m_MetaboxFilePath[metaboxIdentifier] = filePath;
}

OpenViBE::CIdentifier CMetaboxManager::getMetaboxHash(const OpenViBE::CIdentifier& metaboxIdentifier) const
{
	auto resultIt = m_MetaboxHash.find(metaboxIdentifier);
	return resultIt != m_MetaboxHash.end() ? resultIt->second : OV_UndefinedIdentifier;
}

void CMetaboxManager::setMetaboxHash(const OpenViBE::CIdentifier& metaboxIdentifier, const OpenViBE::CIdentifier& hash)
{
	m_MetaboxHash[metaboxIdentifier] = hash;
}
