#include "ovpCBoxAlgorithmGenericStreamReader.h"

#include <iostream>
#include <limits>
#include <fs/Files.h>

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::FileIO;

CBoxAlgorithmGenericStreamReader::CBoxAlgorithmGenericStreamReader(void)
	:m_oReader(*this)
	,m_bHasEBMLHeader(false)
	,m_pFile(NULL)
{
}

uint64 CBoxAlgorithmGenericStreamReader::getClockFrequency(void)
{
	return 128LL<<32; // the box clock frequency
}

boolean CBoxAlgorithmGenericStreamReader::initialize(void)
{
	m_sFilename = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);

	m_bPending=false;

	m_vStreamIndexToOutputIndex.clear();
	m_vStreamIndexToTypeIdentifier.clear();

	return true;
}

boolean CBoxAlgorithmGenericStreamReader::uninitialize(void)
{
	if(m_pFile)
	{
		::fclose(m_pFile);
		m_pFile=NULL;
	}

	return true;
}

boolean CBoxAlgorithmGenericStreamReader::initializeFile()
{

	m_pFile = FS::Files::open(m_sFilename.toASCIIString(), "rb");

	OV_ERROR_UNLESS_KRF(
		m_pFile,
		"Error opening file [" << m_sFilename << "] for reading",
		OpenViBE::Kernel::ErrorType::BadFileRead
	);

	return true;
}

boolean CBoxAlgorithmGenericStreamReader::processClock(IMessageClock& rMessageClock)
{
	getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();

	return true;
}

boolean CBoxAlgorithmGenericStreamReader::process(void)
{
	if(m_pFile == NULL)
	{
		if(!initializeFile())
		{
			return false;
		}
	}
	const IBox& l_rStaticBoxContext=this->getStaticBoxContext();
	IBoxIO& l_rDynamicBoxContext=this->getDynamicBoxContext();

	uint64 l_ui64Time=this->getPlayerContext().getCurrentTime();
	boolean l_bFinished=false;

	while(!l_bFinished && (!::feof(m_pFile) || m_bPending))
	{
		if(m_bPending)
		{
			if(m_ui64EndTime<=l_ui64Time)
			{
				OV_ERROR_UNLESS_KRF(
					m_ui32OutputIndex < l_rStaticBoxContext.getOutputCount(),
					"Stream index " << m_ui32OutputIndex << " can not be output from this box because it does not have enough outputs",
					OpenViBE::Kernel::ErrorType::BadOutput
				);

				l_rDynamicBoxContext.getOutputChunk(m_ui32OutputIndex)->append(m_oPendingChunk);
				l_rDynamicBoxContext.markOutputAsReadyToSend(m_ui32OutputIndex, m_ui64StartTime, m_ui64EndTime);
				m_bPending=false;
			}
			else
			{
				l_bFinished=true;
			}
		}
		else
		{
			boolean l_bJustStarted=true;
			while(!::feof(m_pFile) && m_oReader.getCurrentNodeIdentifier()==EBML::CIdentifier())
			{
				uint8 l_ui8Byte;
				size_t s=::fread(&l_ui8Byte, sizeof(uint8), 1, m_pFile);

				OV_ERROR_UNLESS_KRF(
					s == 1 || l_bJustStarted,
					"Unexpected EOF in " << m_sFilename,
					OpenViBE::Kernel::ErrorType::BadParsing
				);

				m_oReader.processData(&l_ui8Byte, sizeof(l_ui8Byte));
				l_bJustStarted=false;
			}
			if(!::feof(m_pFile) && m_oReader.getCurrentNodeSize()!=0)
			{
				m_oSwap.setSize(m_oReader.getCurrentNodeSize(), true);
				size_t s= (size_t) ::fread(m_oSwap.getDirectPointer(), sizeof(uint8), (size_t) m_oSwap.getSize(), m_pFile);

				OV_ERROR_UNLESS_KRF(
					s == m_oSwap.getSize(),
					"Unexpected EOF in " << m_sFilename,
					OpenViBE::Kernel::ErrorType::BadParsing
				);

				m_oPendingChunk.setSize(0, true);
				m_ui64StartTime = std::numeric_limits<uint64>::max();
				m_ui64EndTime = std::numeric_limits<uint64>::max();
				m_ui32OutputIndex = std::numeric_limits<uint32>::max();

				m_oReader.processData(m_oSwap.getDirectPointer(), m_oSwap.getSize());
			}
		}
	}

	return true;
}

EBML::boolean CBoxAlgorithmGenericStreamReader::isMasterChild(const EBML::CIdentifier& rIdentifier)
{
	if(rIdentifier==EBML_Identifier_Header                        ) return true;
	if(rIdentifier==OVP_NodeId_OpenViBEStream_Header              ) return true;
	if(rIdentifier==OVP_NodeId_OpenViBEStream_Header_Compression  ) return false;
	if(rIdentifier==OVP_NodeId_OpenViBEStream_Header_StreamType  ) return false;
	if(rIdentifier==OVP_NodeId_OpenViBEStream_Buffer              ) return true;
	if(rIdentifier==OVP_NodeId_OpenViBEStream_Buffer_StreamIndex ) return false;
	if(rIdentifier==OVP_NodeId_OpenViBEStream_Buffer_StartTime    ) return false;
	if(rIdentifier==OVP_NodeId_OpenViBEStream_Buffer_EndTime      ) return false;
	if(rIdentifier==OVP_NodeId_OpenViBEStream_Buffer_Content      ) return false;
	return false;
}

void CBoxAlgorithmGenericStreamReader::openChild(const EBML::CIdentifier& rIdentifier)
{
	m_vNodes.push(rIdentifier);

	EBML::CIdentifier& l_rTop=m_vNodes.top();

	if(l_rTop == EBML_Identifier_Header)
	{
		m_bHasEBMLHeader = true;
	}
	if(l_rTop==OVP_NodeId_OpenViBEStream_Header)
	{
		if(!m_bHasEBMLHeader)
		{
			this->getLogManager() << LogLevel_Info << "The file " << m_sFilename << " uses an outdated (but still compatible) version of the .ov file format\n";
		}
	}
	if(l_rTop==OVP_NodeId_OpenViBEStream_Header)
	{
		m_vStreamIndexToOutputIndex.clear();
		m_vStreamIndexToTypeIdentifier.clear();
	}
}

void CBoxAlgorithmGenericStreamReader::processChildData(const void* pBuffer, const uint64 ui64BufferSize)
{
	EBML::CIdentifier& l_rTop=m_vNodes.top();

	// Uncomment this when ebml version will be used
	//if(l_rTop == EBML_Identifier_EBMLVersion)
	//{
	//	const uint64 l_ui64VersionNumber=(uint64)m_oReaderHelper.getUIntegerFromChildData(pBuffer, ui64BufferSize);
	//}

	if(l_rTop==OVP_NodeId_OpenViBEStream_Header_Compression)
	{
		if(m_oReaderHelper.getUIntegerFromChildData(pBuffer, ui64BufferSize) != 0)
		{
			OV_WARNING_K("Impossible to use compression as it is not yet implemented");
		}
	}
	if(l_rTop==OVP_NodeId_OpenViBEStream_Header_StreamType)
	{
		m_vStreamIndexToTypeIdentifier[m_vStreamIndexToTypeIdentifier.size()]=m_oReaderHelper.getUIntegerFromChildData(pBuffer, ui64BufferSize);
	}

	if(l_rTop==OVP_NodeId_OpenViBEStream_Buffer_StreamIndex)
	{
		uint32 l_ui32StreamIndex=(uint32)m_oReaderHelper.getUIntegerFromChildData(pBuffer, ui64BufferSize);
		if(m_vStreamIndexToTypeIdentifier.find(l_ui32StreamIndex)!=m_vStreamIndexToTypeIdentifier.end())
		{
			m_ui32OutputIndex=m_vStreamIndexToOutputIndex[l_ui32StreamIndex];
		}
	}
	if(l_rTop==OVP_NodeId_OpenViBEStream_Buffer_StartTime)
	{
		m_ui64StartTime=m_oReaderHelper.getUIntegerFromChildData(pBuffer, ui64BufferSize);
	}
	if(l_rTop==OVP_NodeId_OpenViBEStream_Buffer_EndTime)
	{
		m_ui64EndTime=m_oReaderHelper.getUIntegerFromChildData(pBuffer, ui64BufferSize);
	}
	if(l_rTop==OVP_NodeId_OpenViBEStream_Buffer_Content)
	{
		m_oPendingChunk.setSize(0, true);
		m_oPendingChunk.append(reinterpret_cast<const EBML::uint8*>(pBuffer), ui64BufferSize);
	}
}

void CBoxAlgorithmGenericStreamReader::closeChild(void)
{
	EBML::CIdentifier& l_rTop=m_vNodes.top();

	if(l_rTop==OVP_NodeId_OpenViBEStream_Header)
	{
		const IBox& l_rStaticBoxContext=this->getStaticBoxContext();

		std::map < uint32, CIdentifier >::const_iterator it;
		std::map < uint32, uint32 > l_vOutputIndexToStreamIndex;

		boolean l_bLostStreams=false;
		boolean l_bLastOutputs=false;

		// Go on each stream of the file
		for(it=m_vStreamIndexToTypeIdentifier.begin(); it!=m_vStreamIndexToTypeIdentifier.end(); ++it)
		{
			CIdentifier l_oOutputTypeIdentifier;
			uint32 l_ui32Index = std::numeric_limits<uint32>::max();

			// Find the first box output with this type that has no file stream connected
			for(uint32 i=0; i<l_rStaticBoxContext.getOutputCount() && l_ui32Index == std::numeric_limits<uint32>::max(); i++)
			{
				if(l_rStaticBoxContext.getOutputType(i, l_oOutputTypeIdentifier))
				{
					if(l_vOutputIndexToStreamIndex.find(i)==l_vOutputIndexToStreamIndex.end())
					{
						if(l_oOutputTypeIdentifier==it->second)
						{
							const CString l_sTypeName=this->getTypeManager().getTypeName(it->second);
							l_ui32Index=i;
						}
					}
				}
			}

			// In case no suitable output was found, see if we can downcast some type
			for(uint32 i=0; i<l_rStaticBoxContext.getOutputCount() && l_ui32Index == std::numeric_limits<uint32>::max(); i++)
			{
				if(l_rStaticBoxContext.getOutputType(i, l_oOutputTypeIdentifier))
				{
					if(l_vOutputIndexToStreamIndex.find(i)==l_vOutputIndexToStreamIndex.end())
					{
						if(this->getTypeManager().isDerivedFromStream(it->second, l_oOutputTypeIdentifier))
						{
							const CString l_sSourceTypeName=this->getTypeManager().getTypeName(it->second);
							const CString l_sOutputTypeName=this->getTypeManager().getTypeName(l_oOutputTypeIdentifier);
							this->getLogManager() << LogLevel_Info << "Note: downcasting output " << i+1 << " from "
								<< l_sSourceTypeName << " to " << l_sOutputTypeName << ", as there is no exactly type-matching output connector.\n";
							l_ui32Index=i;
						}
					}
				}
			}

			// In case it was not found
			if(l_ui32Index == std::numeric_limits<uint32>::max())
			{
				CString l_sTypeName=this->getTypeManager().getTypeName(it->second);

				OV_WARNING_K("No free output connector for stream " << it->first << " of type " << it->second << " (" << l_sTypeName << ")");

				m_vStreamIndexToOutputIndex[it->first] = std::numeric_limits<uint32>::max();
				l_bLostStreams=true;
			}
			else
			{
				m_vStreamIndexToOutputIndex[it->first]=l_ui32Index;
				l_vOutputIndexToStreamIndex[l_ui32Index]=it->first;
			}
		}

		// Warns for output with no stream connected to them
		for(uint32 i=0; i<l_rStaticBoxContext.getOutputCount(); i++)
		{
			if(l_vOutputIndexToStreamIndex.find(i)==l_vOutputIndexToStreamIndex.end())
			{
				OV_WARNING_K("No stream candidate in file for output " << i+1);
				l_bLastOutputs=true;
			}
		}

		// When both outputs and streams were lost, there most probably was a damn mistake
		OV_ERROR_UNLESS_KRV(
			!l_bLastOutputs || !l_bLostStreams,
			"Invalid configuration: missing output for stream(s) and missing stream for output(s)",
			OpenViBE::Kernel::ErrorType::BadConfig
		);
	}

	if(l_rTop==OVP_NodeId_OpenViBEStream_Buffer)
	{
		m_bPending = ((m_ui32OutputIndex != std::numeric_limits<uint32>::max()) &&
					  (m_ui64StartTime != std::numeric_limits<uint64>::max()) &&
					  (m_ui64EndTime != std::numeric_limits<uint64>::max()));
	}

	m_vNodes.pop();
}
