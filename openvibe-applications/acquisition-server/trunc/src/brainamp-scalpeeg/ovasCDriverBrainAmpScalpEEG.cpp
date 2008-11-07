#include "ovasCDriverBrainAmpScalpEEG.h"
#include "../ovasCConfigurationNetworkGlade.h"

#include <system/Time.h>

#include <math.h>

#include <iostream>

#include <stdlib.h>

using namespace OpenViBEAcquisitionServer;
using namespace OpenViBE;
using namespace std;

//___________________________________________________________________//
//                                                                   //

CDriverBrainAmpScalpEEG::CDriverBrainAmpScalpEEG(void)
	:m_pCallback(NULL)
	,m_pConnectionClient(NULL)
	,m_sServerHostName("194.57.166.229")
	,m_ui32ServerHostPort(51244)
	,m_bInitialized(false)
	,m_bStarted(false)
	,m_ui32SampleCountPerSentBlock(0)
	,m_pSample(NULL)
{
}

CDriverBrainAmpScalpEEG::~CDriverBrainAmpScalpEEG(void)
{
}

const char* CDriverBrainAmpScalpEEG::getName(void)
{
	return "BrainAmp Scalp EEG";
}

//___________________________________________________________________//
//                                                                   //

boolean CDriverBrainAmpScalpEEG::initialize(
	const uint32 ui32SampleCountPerSentBlock,
	IDriverCallback& rCallback)
{
	if(m_bInitialized)
	{
		return false;
	}

	// Initialize GUID value
	DEFINE_GUID(GUID_RDAHeader,
		1129858446, 51606, 19590, char(175), char(74), char(152), char(187), char(246), char(201), char(20), char(80)
	);

	// Builds up client connection
	m_pConnectionClient=Socket::createConnectionClient();

	// Tries to connect to server
	m_pConnectionClient->connect(m_sServerHostName, m_ui32ServerHostPort);

	// Checks if connection is correctly established
	if(!m_pConnectionClient->isConnected())
	{
		// In case it is not, try to reconnect
		m_pConnectionClient->connect(m_sServerHostName, m_ui32ServerHostPort);
	}

	if(!m_pConnectionClient->isConnected())
	{
		cout << "Connection problem! Tried 2 times without success! :(" << std::endl;
		cout << "Verify port number and/or Hostname..." << std::endl;
		return false;
	}

	cout << "> Client connected" << std::endl;

	// Initialize vars for reception
	m_pStructRDA_MessageHeader = NULL;
	RDA_MessageHeader l_structRDA_MessageHeader;
	m_pcharStructRDA_MessageHeader = (char*)&l_structRDA_MessageHeader;

	uint32 l_ui32Received = 0;
	uint32 l_ui32ReqLength = 0;
	uint32 l_ui32Result = 0;
	uint32 l_ui32Datasize = sizeof(l_structRDA_MessageHeader);

	// Receive Header
	while(l_ui32Received < l_ui32Datasize)
	{
		l_ui32ReqLength = l_ui32Datasize -  l_ui32Received;
		l_ui32Result = m_pConnectionClient->receiveBuffer((char*)m_pcharStructRDA_MessageHeader, l_ui32ReqLength);

		l_ui32Received += l_ui32Result;
		m_pcharStructRDA_MessageHeader += l_ui32Result;
	}

	// Check for correct header GUID.
	if (!COMPARE_GUID(l_structRDA_MessageHeader.guid, GUID_RDAHeader))
	{
		cout << "GUID received is not correct!" << std::endl;
		return false;
	}

	// Check for correct header nType
	if (l_structRDA_MessageHeader.nType !=1)
	{
		cout << "First Message received is not an header!" << std::endl;
		cout << "Try to reconnect...." << std::endl;
		return false;
	}

	// Retrieve rest of data
	if (!(*(&m_pStructRDA_MessageHeader) = (RDA_MessageHeader*)malloc(l_structRDA_MessageHeader.nSize)))
	{
		cout << "Couldn't allocate memory" << std::endl;
		return -1;
	}
	else
	{
		memcpy(*(&m_pStructRDA_MessageHeader), &l_structRDA_MessageHeader, sizeof(l_structRDA_MessageHeader));
		m_pcharStructRDA_MessageHeader = (char*)(*(&m_pStructRDA_MessageHeader)) + sizeof(l_structRDA_MessageHeader);
		l_ui32Received=0;
		l_ui32ReqLength = 0;
		l_ui32Result = 0;
		l_ui32Datasize = l_structRDA_MessageHeader.nSize - sizeof(l_structRDA_MessageHeader);
		while(l_ui32Received < l_ui32Datasize)
		{
			l_ui32ReqLength = l_ui32Datasize -  l_ui32Received;
			l_ui32Result = m_pConnectionClient->receiveBuffer((char*)m_pcharStructRDA_MessageHeader, l_ui32ReqLength);

			l_ui32Received += l_ui32Result;
			m_pcharStructRDA_MessageHeader += l_ui32Result;
		}
	}

	m_pStructRDA_MessageStart = NULL;
	m_pStructRDA_MessageStart = (RDA_MessageStart*)m_pStructRDA_MessageHeader;

	cout << "> Header received" << std::endl;

	// Save Header info into m_oHeader
	//m_oHeader.setExperimentIdentifier();
	//m_oHeader.setExperimentDate();

	//m_oHeader.setSubjectId();
	//m_oHeader.setSubjectName();
	//m_oHeader.setSubjectAge(m_structHeader.subjectAge);
	//m_oHeader.setSubjectGender();

	//m_oHeader.setLaboratoryId();
	//m_oHeader.setLaboratoryName();

	//m_oHeader.setTechnicianId();
	//m_oHeader.setTechnicianName();

	m_oHeader.setChannelCount((uint32)m_pStructRDA_MessageStart->nChannels);

	char* l_pszChannelNames = (char*)m_pStructRDA_MessageStart->dResolutions + m_pStructRDA_MessageStart->nChannels * sizeof(m_pStructRDA_MessageStart->dResolutions[0]);
	for(uint32 i=0; i<m_oHeader.getChannelCount(); i++)
	{
		m_oHeader.setChannelName(i, l_pszChannelNames);
		m_oHeader.setChannelGain(i, (m_pStructRDA_MessageStart->dResolutions[i]));
		l_pszChannelNames += strlen(l_pszChannelNames) + 1;
	}

	m_oHeader.setSamplingFrequency((uint32)(1000000/m_pStructRDA_MessageStart->dSamplingInterval)); //dSamplingInterval in microseconds

	m_ui32SampleCountPerSentBlock=ui32SampleCountPerSentBlock;

	m_pSample=new float32[m_oHeader.getChannelCount()*m_ui32SampleCountPerSentBlock];

	if(!m_pSample)
	{
		delete [] m_pSample;
		m_pSample=NULL;
		return false;
	}

	m_pCallback=&rCallback;

	m_ui32IndexIn = 0;
	m_ui32IndexOut = 0;
	m_ui32BuffDataIndex = 0;

	m_ui32DataOffset =0;
	m_ui32MarkerCount =0;
	m_ui32NumberOfMarkers = 0;
	m_bInitialized=true;

	return m_bInitialized;
}

boolean CDriverBrainAmpScalpEEG::start(void)
{
	if(!m_bInitialized)
	{
		return false;
	}

	if(m_bStarted)
	{
		return false;
	}

	m_bStarted=true;

	return m_bStarted;

}

boolean CDriverBrainAmpScalpEEG::loop(void)
{
	if(!m_bInitialized)
	{
		return false;
	}

	if(!m_bStarted)
	{
		return false;
	}

	DEFINE_GUID(GUID_RDAHeader,
		1129858446, 51606, 19590, char(175), char(74), char(152), char(187), char(246), char(201), char(20), char(80)
	);

	// Initialize var to receive buffer of data
	m_pStructRDA_MessageHeader = NULL;
	RDA_MessageHeader l_structRDA_MessageHeader;
	m_pcharStructRDA_MessageHeader = (char*)&l_structRDA_MessageHeader;

	uint32 l_ui32Received = 0;
	uint32 l_ui32ReqLength = 0;
	uint32 l_ui32Result = 0;
	uint32 l_ui32Datasize = sizeof(l_structRDA_MessageHeader);

	// Receive Header
	while(l_ui32Received < l_ui32Datasize)
	{
		l_ui32ReqLength = l_ui32Datasize -  l_ui32Received;
		l_ui32Result = m_pConnectionClient->receiveBuffer((char*)m_pcharStructRDA_MessageHeader, l_ui32ReqLength);

		l_ui32Received += l_ui32Result;
		m_pcharStructRDA_MessageHeader += l_ui32Result;
	}

	// Check for correct header nType
	if (l_structRDA_MessageHeader.nType == 1)
	{
		cout << "Message received is a header!" << std::endl;
		m_bStarted = false;
		return false;
	}
	if (l_structRDA_MessageHeader.nType == 3)
	{
		cout << "Message received is a STOP!" << std::endl;
		m_bStarted = false;
		return false;
	}
	if (l_structRDA_MessageHeader.nType !=4)
	{
		return false;
	}
	// Check for correct header GUID.
	if (!COMPARE_GUID(l_structRDA_MessageHeader.guid, GUID_RDAHeader))
	{
		cout << "GUID received is not correct!" << std::endl;
		return false;
	}

	// Retrieve rest of block.
	if (!(*(&m_pStructRDA_MessageHeader) = (RDA_MessageHeader*)malloc(l_structRDA_MessageHeader.nSize)))
	{
		cout << "Couldn't allocate memory" << std::endl;
		return -1;
	}
	else
	{
		memcpy(*(&m_pStructRDA_MessageHeader), &l_structRDA_MessageHeader, sizeof(l_structRDA_MessageHeader));
		m_pcharStructRDA_MessageHeader = (char*)(*(&m_pStructRDA_MessageHeader)) + sizeof(l_structRDA_MessageHeader);
		l_ui32Received=0;
		l_ui32ReqLength = 0;
		l_ui32Result = 0;
		l_ui32Datasize = l_structRDA_MessageHeader.nSize - sizeof(l_structRDA_MessageHeader);
		while(l_ui32Received < l_ui32Datasize)
		{
			l_ui32ReqLength = l_ui32Datasize -  l_ui32Received;
			l_ui32Result = m_pConnectionClient->receiveBuffer((char*)m_pcharStructRDA_MessageHeader, l_ui32ReqLength);

			l_ui32Received += l_ui32Result;
			m_pcharStructRDA_MessageHeader += l_ui32Result;
		}
		m_ui32BuffDataIndex++;
	}

	// Put the data into MessageData32 structure
	m_pStructRDA_MessageData32 = NULL;
	m_pStructRDA_MessageData32 = (RDA_MessageData32*)m_pStructRDA_MessageHeader;

	//////////////////////
	//Markers
	if (m_pStructRDA_MessageData32->nMarkers > 0)
	{
		if (m_pStructRDA_MessageData32->nMarkers == 0)
		{
			return true;
		}

		m_pStructRDA_Marker = (RDA_Marker*)((char*)m_pStructRDA_MessageData32->fData + m_pStructRDA_MessageData32->nPoints * m_oHeader.getChannelCount() * sizeof(m_pStructRDA_MessageData32->fData[0]));

		m_ui32NumberOfMarkers = m_pStructRDA_MessageData32->nMarkers;
	
		m_vStimulationIdentifier.assign(m_ui32NumberOfMarkers, 0);
		m_vStimulationDate.assign(m_ui32NumberOfMarkers, 0);
		m_vStimulationSample.assign(m_ui32NumberOfMarkers, 0);
		
		for (uint32 i = 0; i < m_pStructRDA_MessageData32->nMarkers; i++)
		{
			char* pszType = m_pStructRDA_Marker->sTypeDesc;
			char* pszDescription = pszType + strlen(pszType) + 1;
			
			cout << "Stim n�" << m_ui32MarkerCount + i + 1 << ", " << atoi(strtok (pszDescription,"S")) << ", " << m_pStructRDA_Marker->nPosition + m_ui32DataOffset<< std::endl;
			
			m_vStimulationIdentifier[i] = atoi(strtok (pszDescription,"S"));
			m_vStimulationDate[i] = (((uint64)(m_pStructRDA_Marker->nPosition + m_ui32DataOffset)) << 32) / m_oHeader.getSamplingFrequency();
			m_vStimulationSample[i] = m_pStructRDA_Marker->nPosition+ m_ui32DataOffset;
			m_pStructRDA_Marker = (RDA_Marker*)((char*)m_pStructRDA_Marker + m_pStructRDA_Marker->nSize);
		}

		m_ui32MarkerCount += m_pStructRDA_MessageData32->nMarkers;
	}

	m_ui32DataOffset += m_pStructRDA_MessageData32->nPoints;
	//////////////////////

	// if input flow is equal to output one
	if (m_ui32SampleCountPerSentBlock == (uint32)m_pStructRDA_MessageData32->nPoints)
	{
		for (uint32 i=0; i < m_oHeader.getChannelCount(); i++)
		{
			for (uint32 j=0; j < m_ui32SampleCountPerSentBlock; j++)
			{
				m_pSample[j + i*m_ui32SampleCountPerSentBlock] = (float32)m_pStructRDA_MessageData32->fData[m_oHeader.getChannelCount()*j + i]*m_oHeader.getChannelGain(i);
			}
		}

		// send data
		CStimulationSet l_oStimulationSet;
		l_oStimulationSet.setStimulationCount(m_ui32NumberOfMarkers);
		for (uint32 i = 0; i < m_ui32NumberOfMarkers; i++)
		{
			l_oStimulationSet.setStimulationIdentifier(i, m_vStimulationIdentifier[i]);
			l_oStimulationSet.setStimulationDate(i, m_vStimulationDate[i]);
			l_oStimulationSet.setStimulationDuration(i, 0);
		}
		
		m_pCallback->setSamples(m_pSample);
		m_pCallback->setStimulationSet(l_oStimulationSet);
		m_ui32NumberOfMarkers = 0;
		
	}
	else
	{
		// if output flow is bigger
		if (m_ui32SampleCountPerSentBlock > (uint32)m_pStructRDA_MessageData32->nPoints)
		{
			while (m_ui32IndexOut + (uint32)m_pStructRDA_MessageData32->nPoints < m_ui32SampleCountPerSentBlock)
			{
				for (uint32 i=0; i < m_oHeader.getChannelCount(); i++)
				{
					for (uint32 j=0; j < (uint32)m_pStructRDA_MessageData32->nPoints; j++)
					{
						m_pSample[m_ui32IndexOut + j + i*m_ui32SampleCountPerSentBlock] = (float32)m_pStructRDA_MessageData32->fData[m_oHeader.getChannelCount()*j + i]*m_oHeader.getChannelGain(i);
					}
				}

				m_ui32IndexOut = m_ui32IndexOut + (uint32)m_pStructRDA_MessageData32->nPoints;

				// Initialize var to receive buffer of data
				m_pStructRDA_MessageHeader = NULL;
				RDA_MessageHeader l_structRDA_MessageHeader;
				m_pcharStructRDA_MessageHeader = (char*)&l_structRDA_MessageHeader;

				uint32 l_ui32Received = 0;
				uint32 l_ui32ReqLength = 0;
				uint32 l_ui32Result = 0;
				uint32 l_ui32Datasize = sizeof(l_structRDA_MessageHeader);

				// Receive Header
				while(l_ui32Received < l_ui32Datasize)
				{
					l_ui32ReqLength = l_ui32Datasize -  l_ui32Received;
					l_ui32Result = m_pConnectionClient->receiveBuffer((char*)m_pcharStructRDA_MessageHeader, l_ui32ReqLength);

					l_ui32Received += l_ui32Result;
					m_pcharStructRDA_MessageHeader += l_ui32Result;
				}

				// Check for correct header nType
				if (l_structRDA_MessageHeader.nType == 1)
				{
					cout << "Message received is a header!" << std::endl;
					m_bStarted = false;
					return false;
				}
				if (l_structRDA_MessageHeader.nType == 3)
				{
					cout << "Message received is a STOP!" << std::endl;
					m_bStarted = false;
					return false;
				}
				if (l_structRDA_MessageHeader.nType !=4)
				{
					return false;
				}
				// Check for correct header GUID.
				if (!COMPARE_GUID(l_structRDA_MessageHeader.guid, GUID_RDAHeader))
				{
					cout << "GUID received is not correct!" << std::endl;
					return false;
				}

				//Retrieve rest of data
				if (!(*(&m_pStructRDA_MessageHeader) = (RDA_MessageHeader*)malloc(l_structRDA_MessageHeader.nSize)))
				{
					cout << "Couldn't allocate memory" << std::endl;
					return -1;
				}
				else
				{
					memcpy(*(&m_pStructRDA_MessageHeader), &l_structRDA_MessageHeader, sizeof(l_structRDA_MessageHeader));
					m_pcharStructRDA_MessageHeader = (char*)(*(&m_pStructRDA_MessageHeader)) + sizeof(l_structRDA_MessageHeader);
					l_ui32Received=0;
					l_ui32ReqLength = 0;
					l_ui32Result = 0;
					l_ui32Datasize = l_structRDA_MessageHeader.nSize - sizeof(l_structRDA_MessageHeader);
					while(l_ui32Received < l_ui32Datasize)
					{
						l_ui32ReqLength = l_ui32Datasize -  l_ui32Received;
						l_ui32Result = m_pConnectionClient->receiveBuffer((char*)m_pcharStructRDA_MessageHeader, l_ui32ReqLength);

						l_ui32Received += l_ui32Result;
						m_pcharStructRDA_MessageHeader += l_ui32Result;
					}
					m_ui32BuffDataIndex++;
				}

				m_pStructRDA_MessageData32 = NULL;
				m_pStructRDA_MessageData32 = (RDA_MessageData32*)m_pStructRDA_MessageHeader;

				//////////////////////
				//Markers
				if (m_pStructRDA_MessageData32->nMarkers > 0)
				{
					if (m_pStructRDA_MessageData32->nMarkers == 0)
					{
						return true;
					}

					m_pStructRDA_Marker = (RDA_Marker*)((char*)m_pStructRDA_MessageData32->fData + m_pStructRDA_MessageData32->nPoints * m_oHeader.getChannelCount() * sizeof(m_pStructRDA_MessageData32->fData[0]));

					//save stimulation informations into temporary tables
					OpenViBE::uint32 l_ui32NumberOfMarkers;
					l_ui32NumberOfMarkers = m_ui32NumberOfMarkers;
					
					m_ui32NumberOfMarkers += m_pStructRDA_MessageData32->nMarkers;
					
					m_vStimulationIdentifier.resize(m_ui32NumberOfMarkers, 0);
					m_vStimulationDate.resize(m_ui32NumberOfMarkers, 0);
					m_vStimulationSample.resize(m_ui32NumberOfMarkers, 0);
					
					for (uint32 i = 0; i < m_pStructRDA_MessageData32->nMarkers; i++)
					{
						char* pszType = m_pStructRDA_Marker->sTypeDesc;
						char* pszDescription = pszType + strlen(pszType) + 1;

						cout << "Stim n�" << m_ui32MarkerCount + i + 1 << ", " << atoi(strtok (pszDescription,"S")) << ", " << m_pStructRDA_Marker->nPosition + m_ui32DataOffset<< std::endl;

						m_vStimulationIdentifier[i+l_ui32NumberOfMarkers] = atoi(strtok (pszDescription,"S"));
						m_vStimulationDate[i+l_ui32NumberOfMarkers] = (((uint64)(m_pStructRDA_Marker->nPosition + m_ui32DataOffset)) << 32) / m_oHeader.getSamplingFrequency();
						m_vStimulationSample[i+l_ui32NumberOfMarkers] = m_pStructRDA_Marker->nPosition + m_ui32DataOffset;
						m_pStructRDA_Marker = (RDA_Marker*)((char*)m_pStructRDA_Marker + m_pStructRDA_Marker->nSize);
					}
					m_ui32MarkerCount += m_pStructRDA_MessageData32->nMarkers;
				}

				m_ui32DataOffset += m_pStructRDA_MessageData32->nPoints;;
				//////////////////////
			}

			// Finishing filling up
			for (uint32 i=0; i < m_oHeader.getChannelCount(); i++)
			{
				for (uint32 j=0; j < m_ui32SampleCountPerSentBlock-m_ui32IndexOut ; j++)
				{
					m_pSample[m_ui32IndexOut + j + i*m_ui32SampleCountPerSentBlock] = (float32)m_pStructRDA_MessageData32->fData[m_oHeader.getChannelCount()*j + i] *m_oHeader.getChannelGain(i);
				}
			}
			
			// Verifying inclusion of markers into output data size
			uint32 l_ui32NumberOfMarkersToSend = 0;
			for (uint32 i = 0; i < m_ui32NumberOfMarkers; i++)
			{
				if (m_vStimulationSample[i] < m_ui32SampleCountPerSentBlock)
				{
					l_ui32NumberOfMarkersToSend++;
				}
			}
			cout << "l_ui32NumberOfMarkersToSend = " << l_ui32NumberOfMarkersToSend<<endl;
			
			// send buffers
			CStimulationSet l_oStimulationSet;
			l_oStimulationSet.setStimulationCount(l_ui32NumberOfMarkersToSend);
			for (uint32 i = 0; i < l_ui32NumberOfMarkersToSend; i++)
			{
				l_oStimulationSet.setStimulationIdentifier(i, m_vStimulationIdentifier[i]);
				l_oStimulationSet.setStimulationDate(i, m_vStimulationDate[i]);
				l_oStimulationSet.setStimulationDuration(i, 0);
			}
			
			// send data
			m_pCallback->setSamples(m_pSample);
			m_pCallback->setStimulationSet(l_oStimulationSet);
			
			// save the rest of markers
			std::vector<OpenViBE::uint32> l_vStimulationIdentifier;
			std::vector<OpenViBE::uint64> l_vStimulationDate;
			std::vector<OpenViBE::uint64> l_vStimulationSample;
			l_vStimulationIdentifier.assign(m_ui32NumberOfMarkers-l_ui32NumberOfMarkersToSend, 0);
			l_vStimulationDate.assign(m_ui32NumberOfMarkers-l_ui32NumberOfMarkersToSend, 0);
			l_vStimulationSample.assign(m_ui32NumberOfMarkers-l_ui32NumberOfMarkersToSend, 0);
			for (uint32 i = l_ui32NumberOfMarkersToSend; i < m_ui32NumberOfMarkers; i++)
			{
				l_vStimulationDate[i-l_ui32NumberOfMarkersToSend] = m_vStimulationDate[i] - m_ui32SampleCountPerSentBlock%(uint32)m_pStructRDA_MessageData32->nPoints;
				l_vStimulationIdentifier[i-l_ui32NumberOfMarkersToSend] = m_vStimulationIdentifier[i];
				l_vStimulationSample[i-l_ui32NumberOfMarkersToSend] = m_vStimulationSample[i] - m_ui32SampleCountPerSentBlock;
			}
			m_vStimulationIdentifier.resize( m_ui32NumberOfMarkers - l_ui32NumberOfMarkersToSend);
			m_vStimulationDate.resize( m_ui32NumberOfMarkers - l_ui32NumberOfMarkersToSend);
			m_vStimulationSample.resize( m_ui32NumberOfMarkers - l_ui32NumberOfMarkersToSend);
			for (uint32 i = 0; i < m_ui32NumberOfMarkers - l_ui32NumberOfMarkersToSend; i++)
			{
				m_vStimulationDate[i] = l_vStimulationDate[i];
				m_vStimulationIdentifier[i] = l_vStimulationIdentifier[i];
				m_vStimulationSample[i] = l_vStimulationSample[i];
			}
			
			m_ui32DataOffset = m_ui32SampleCountPerSentBlock%(uint32)m_pStructRDA_MessageData32->nPoints;
			m_ui32NumberOfMarkers -= l_ui32NumberOfMarkersToSend;
			//~ m_ui32DataOffset=0;
			//~ m_ui32NumberOfMarkers = 0;
				
			// Reset index out because new output
			m_ui32IndexIn = m_ui32SampleCountPerSentBlock-m_ui32IndexOut;
			m_ui32IndexOut = 0;

			// Save the rest of the buffer input
			for (uint32 i=0; i < m_oHeader.getChannelCount(); i++)
			{
				for (uint32 j=0; j < (uint32)m_pStructRDA_MessageData32->nPoints - m_ui32IndexIn ; j++)
				{
					m_pSample[j + i*m_ui32SampleCountPerSentBlock] = (float32)m_pStructRDA_MessageData32->fData[m_ui32IndexIn *m_oHeader.getChannelCount() + m_oHeader.getChannelCount()*j + i] *m_oHeader.getChannelGain(i);
				}
			}

			m_ui32IndexOut = (uint32)m_pStructRDA_MessageData32->nPoints-m_ui32IndexIn;
			m_ui32IndexIn = 0;
		
		}
		else
		{
			// if output flow is smaller
			while (m_ui32IndexIn + (m_ui32SampleCountPerSentBlock - m_ui32IndexOut)< (uint32)m_pStructRDA_MessageData32->nPoints)
			{
				for (uint32 i=0; i < m_oHeader.getChannelCount(); i++)
				{
					for (uint32 j=0; j < m_ui32SampleCountPerSentBlock - m_ui32IndexOut; j++)
					{
						m_pSample[j + m_ui32IndexOut+ i*m_ui32SampleCountPerSentBlock] = (float32)m_pStructRDA_MessageData32->fData[m_oHeader.getChannelCount() *  m_ui32IndexIn + m_oHeader.getChannelCount()*j + i]  *m_oHeader.getChannelGain(i);
					}
				}
					
				// Verifying inclusion of markers into output data size
				uint32 l_ui32NumberOfMarkersToSend = 0;
				for (uint32 i = 0; i < m_ui32NumberOfMarkers; i++)
				{
					if (m_vStimulationSample[i]<m_ui32SampleCountPerSentBlock)
					{
						l_ui32NumberOfMarkersToSend++;
					}
				}
				
				CStimulationSet l_oStimulationSet;
				l_oStimulationSet.setStimulationCount(l_ui32NumberOfMarkersToSend);
				for (uint32 i = 0; i < l_ui32NumberOfMarkersToSend; i++)
				{
					l_oStimulationSet.setStimulationIdentifier(i, m_vStimulationIdentifier[i]);
					l_oStimulationSet.setStimulationDate(i, m_vStimulationDate[i]);
					l_oStimulationSet.setStimulationDuration(i, 0);
				}
		
				//send data
				m_pCallback->setSamples(m_pSample);
				m_pCallback->setStimulationSet(l_oStimulationSet);
				
				
				// save the rest of markers
				std::vector<OpenViBE::uint32> l_vStimulationIdentifier;
				std::vector<OpenViBE::uint64> l_vStimulationDate;
				std::vector<OpenViBE::uint64> l_vStimulationSample;
				l_vStimulationIdentifier.assign(m_ui32NumberOfMarkers-l_ui32NumberOfMarkersToSend, 0);
				l_vStimulationDate.assign(m_ui32NumberOfMarkers-l_ui32NumberOfMarkersToSend, 0);
				l_vStimulationSample.assign(m_ui32NumberOfMarkers-l_ui32NumberOfMarkersToSend, 0);
				for (uint32 i = l_ui32NumberOfMarkersToSend; i < m_ui32NumberOfMarkers; i++)
				{
					l_vStimulationDate[i-l_ui32NumberOfMarkersToSend] = m_vStimulationDate[i] - m_ui32SampleCountPerSentBlock%(uint32)m_pStructRDA_MessageData32->nPoints;
					l_vStimulationIdentifier[i-l_ui32NumberOfMarkersToSend] = m_vStimulationIdentifier[i];
					l_vStimulationSample[i-l_ui32NumberOfMarkersToSend] = m_vStimulationSample[i] - m_ui32SampleCountPerSentBlock;
				}
				m_vStimulationIdentifier.resize( m_ui32NumberOfMarkers - l_ui32NumberOfMarkersToSend);
				m_vStimulationDate.resize( m_ui32NumberOfMarkers - l_ui32NumberOfMarkersToSend);
				m_vStimulationSample.resize( m_ui32NumberOfMarkers - l_ui32NumberOfMarkersToSend);
				for (uint32 i = 0; i < m_ui32NumberOfMarkers - l_ui32NumberOfMarkersToSend; i++)
				{
					m_vStimulationDate[i] = l_vStimulationDate[i];
					m_vStimulationIdentifier[i] = l_vStimulationIdentifier[i];
					m_vStimulationSample[i] = l_vStimulationSample[i];
				}
				
				m_ui32NumberOfMarkers -= l_ui32NumberOfMarkersToSend;
					
				m_ui32IndexIn = m_ui32IndexIn + (m_ui32SampleCountPerSentBlock - m_ui32IndexOut);
				m_ui32IndexOut = 0;
			}
			
			// save the rest of buff data
			for (uint32 i=0; i < m_oHeader.getChannelCount(); i++)
			{
				for (uint32 j=0; j <  (uint32)m_pStructRDA_MessageData32->nPoints - m_ui32IndexIn; j++)
				{
					m_pSample[j + m_ui32IndexOut+ i*m_ui32SampleCountPerSentBlock] = (float32)m_pStructRDA_MessageData32->fData[m_oHeader.getChannelCount() *  m_ui32IndexIn + m_oHeader.getChannelCount()*j + i]  *m_oHeader.getChannelGain(i);
				}
			}

			m_ui32IndexOut = (uint32)m_pStructRDA_MessageData32->nPoints - m_ui32IndexIn;
			m_ui32IndexIn = 0;
			
			
		}
	}

	return true;

}

boolean CDriverBrainAmpScalpEEG::stop(void)
{
	cout << "> Connection stopped" << std::endl;

	if(!m_bInitialized)
	{
		return false;
	}

	if(!m_bStarted)
	{
		return false;
	}

	m_bStarted=false;
	return !m_bStarted;
}

boolean CDriverBrainAmpScalpEEG::uninitialize(void)
{
	if(!m_bInitialized)
	{
		return false;
	}

	if(m_bStarted)
	{
		return false;
	}

	m_bInitialized=false;

	if (m_pcharStructRDA_MessageHeader!=NULL) m_pcharStructRDA_MessageHeader=NULL;
	if (m_pStructRDA_MessageHeader!=NULL) m_pStructRDA_MessageHeader= NULL;
	if (m_pStructRDA_MessageStart!=NULL) m_pStructRDA_MessageStart=NULL;
	if (m_pStructRDA_MessageStop!=NULL) m_pStructRDA_MessageStop=NULL;
	if (m_pStructRDA_MessageData32!=NULL) m_pStructRDA_MessageData32=NULL;
	if (m_pStructRDA_Marker!=NULL) m_pStructRDA_Marker=NULL;

	delete [] m_pSample;
	m_pSample=NULL;
	m_pCallback=NULL;

	// Cleans up client connection
	m_pConnectionClient->close();
	m_pConnectionClient->release();
	m_pConnectionClient=NULL;
	cout << "> Client disconnected" << std::endl;

	return true;
}

//___________________________________________________________________//
//                                                                   //

boolean CDriverBrainAmpScalpEEG::isConfigurable(void)
{
	return true;
}

boolean CDriverBrainAmpScalpEEG::configure(void)
{
	CConfigurationNetworkGlade l_oConfiguration("../share/openvibe-applications/acquisition-server/interface-BrainAmp-ScalpEEG.glade");

	l_oConfiguration.setHostName(m_sServerHostName);
	l_oConfiguration.setHostPort(m_ui32ServerHostPort);

	if(l_oConfiguration.configure(m_oHeader))
	{
		m_sServerHostName=l_oConfiguration.getHostName();
		m_ui32ServerHostPort=l_oConfiguration.getHostPort();
		return true;
	}

	return false;
}
