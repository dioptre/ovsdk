#include "ovpCBoxAlgorithmCSVFileWriter.h"

#include <string>
#include <iostream>

#include "openvibe/ovITimeArithmetics.h"

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::FileIO;

CBoxAlgorithmCSVFileWriter::CBoxAlgorithmCSVFileWriter(void)
	:
	m_fpRealProcess(NULL)
	,m_pStreamDecoder(NULL)
	,m_pMatrix(NULL)
	,m_bDeleteMatrix(false)
{
}

boolean CBoxAlgorithmCSVFileWriter::initialize(void)
{
	this->getStaticBoxContext().getInputType(0, m_oTypeIdentifier);

	const CString l_sFilename=FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
	m_sSeparator=FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);
	const uint64 l_ui64Precision=FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2);

	m_oFileStream.open(l_sFilename.toASCIIString(), std::ios::trunc);
	if(!m_oFileStream)
	{
		this->getLogManager() << LogLevel_ImportantWarning << "Could not open file [" << l_sFilename << "] for writing\n";
		return false;
	}

	m_oFileStream << std::scientific;
	m_oFileStream.precision(static_cast<std::streamsize>(l_ui64Precision));

	if(this->getTypeManager().isDerivedFromStream(m_oTypeIdentifier, OV_TypeId_StreamedMatrix))
	{
		if(m_oTypeIdentifier==OV_TypeId_Signal)
		{
			m_pStreamDecoder=new OpenViBEToolkit::TSignalDecoder < CBoxAlgorithmCSVFileWriter >();
			m_pStreamDecoder->initialize(*this,0);
		}
		else if(m_oTypeIdentifier==OV_TypeId_Spectrum)
		{
			m_pStreamDecoder=new OpenViBEToolkit::TSpectrumDecoder < CBoxAlgorithmCSVFileWriter >();
			m_pStreamDecoder->initialize(*this,0);
		}
		else if(m_oTypeIdentifier==OV_TypeId_FeatureVector)
		{
			m_pStreamDecoder=new OpenViBEToolkit::TFeatureVectorDecoder  < CBoxAlgorithmCSVFileWriter >();
			m_pStreamDecoder->initialize(*this, 0);
		}
		else
		{
			if(m_oTypeIdentifier!=OV_TypeId_StreamedMatrix)
			{
				this->getLogManager() << LogLevel_Info << "Input is a type derived from matrix that the box doesn't recognize, decoding as Streamed Matrix\n";
			}
			m_pStreamDecoder=new OpenViBEToolkit::TStreamedMatrixDecoder < CBoxAlgorithmCSVFileWriter >();
			m_pStreamDecoder->initialize(*this,0);
		}
		m_fpRealProcess=&CBoxAlgorithmCSVFileWriter::process_streamedMatrix;
	}
	else if(m_oTypeIdentifier==OV_TypeId_Stimulations)
	{
		m_pStreamDecoder=new OpenViBEToolkit::TStimulationDecoder < CBoxAlgorithmCSVFileWriter >();
		m_pStreamDecoder->initialize(*this,0);
		m_fpRealProcess=&CBoxAlgorithmCSVFileWriter::process_stimulation;
	}
	else
	{
		this->getLogManager() << LogLevel_ImportantWarning << "Invalid input type identifier " << this->getTypeManager().getTypeName(m_oTypeIdentifier) << "\n";
		return false;
	}

	m_ui64SampleCount = 0;

	m_bFirstBuffer=true;
	return true;
}

boolean CBoxAlgorithmCSVFileWriter::uninitialize(void)
{
	if(m_oFileStream.is_open())
	{
		m_oFileStream.close();
	}

	if(m_bDeleteMatrix)
	{
		delete m_pMatrix;
		m_pMatrix = NULL;
	}

	if(m_pStreamDecoder)
	{
		m_pStreamDecoder->uninitialize();
		delete m_pStreamDecoder;
	}

	return true;
}

boolean CBoxAlgorithmCSVFileWriter::processInput(uint32 ui32InputIndex)
{
	getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
	return true;
}

boolean CBoxAlgorithmCSVFileWriter::process(void)
{
	return (this->*m_fpRealProcess)();
}

boolean CBoxAlgorithmCSVFileWriter::process_streamedMatrix(void)
{
	IBoxIO& l_rDynamicBoxContext=this->getDynamicBoxContext();
	for(uint32 i=0; i<l_rDynamicBoxContext.getInputChunkCount(0); i++)
	{
		const uint64 l_ui64StartTime=l_rDynamicBoxContext.getInputChunkStartTime(0, i);
		const uint64 l_ui64EndTime=l_rDynamicBoxContext.getInputChunkEndTime(0, i);
		m_pStreamDecoder->decode(i);
		IMatrix* l_pMatrix = ((OpenViBEToolkit::TStreamedMatrixDecoder < CBoxAlgorithmCSVFileWriter >*)m_pStreamDecoder)->getOutputMatrix();
		if(m_pStreamDecoder->isHeaderReceived())
		{
			if(l_pMatrix->getDimensionCount() > 2 || l_pMatrix->getDimensionCount() < 1)
			{
				this->getLogManager() << LogLevel_ImportantWarning << "Input matrix does not have 1 or 2 dimensions - Cannot write content in CSV file...\n";
				return false;
			}

			// The matrix is a vector, make a [n x 1] matrix to represent it
			if( l_pMatrix->getDimensionCount() == 1 )
			{
				m_pMatrix = new CMatrix();
				m_bDeleteMatrix = true;
				m_pMatrix->setDimensionCount(2);

				if(m_oTypeIdentifier==OV_TypeId_FeatureVector)
				{
					// Flip, [n channels X 1 sample]
					m_pMatrix->setDimensionSize(0,l_pMatrix->getDimensionSize(0));
					m_pMatrix->setDimensionSize(1,1);
					for(uint32 i=0;i<l_pMatrix->getDimensionSize(0);i++)
					{
						m_pMatrix->setDimensionLabel(0,i,l_pMatrix->getDimensionLabel(0,i));
					}
				} 
				else
				{
					// As-is, [1 channel X n samples]
					m_pMatrix->setDimensionSize(0,1);
					m_pMatrix->setDimensionSize(1,l_pMatrix->getDimensionSize(0));
					for(uint32 i=0;i<l_pMatrix->getDimensionSize(0);i++)
					{
						m_pMatrix->setDimensionLabel(1,i,l_pMatrix->getDimensionLabel(0,i));
					}
				}

			}
			else
			{
				m_pMatrix=l_pMatrix;
			}
//			std::cout<<&m_pMatrix<<" "<<&op_pMatrix<<"\n";
			m_oFileStream << "Time (s)";
			for(uint32 c=0; c<m_pMatrix->getDimensionSize(0); c++)
			{
				std::string l_sLabel(m_pMatrix->getDimensionLabel(0, c));
				while(l_sLabel.length()>0 && l_sLabel[l_sLabel.length()-1]==' ')
				{
					l_sLabel.erase(l_sLabel.length()-1);
				}
				m_oFileStream << m_sSeparator.toASCIIString() << l_sLabel.c_str();
			}

			if(m_oTypeIdentifier==OV_TypeId_Signal)
			{
				m_oFileStream << m_sSeparator.toASCIIString() << "Sampling Rate";
			}
			else if(m_oTypeIdentifier==OV_TypeId_Spectrum)
			{
				m_oFileStream << m_sSeparator << "Min frequency band";
				m_oFileStream << m_sSeparator << "Max frequency band";
			}
			else
			{
			}

			m_oFileStream << "\n";
		}
		if(m_pStreamDecoder->isBufferReceived())
		{
			const uint32 l_ui32NumChannels = m_pMatrix->getDimensionSize(0);
			const uint32 l_ui32NumSamples = m_pMatrix->getDimensionSize(1);

			for(uint32 s=0; s<l_ui32NumSamples; s++)
			{
				if(m_oTypeIdentifier==OV_TypeId_StreamedMatrix || m_oTypeIdentifier==OV_TypeId_FeatureVector)
				{
					m_oFileStream << ITimeArithmetics::timeToSeconds(l_ui64StartTime);
				}
				else if(m_oTypeIdentifier==OV_TypeId_Signal)
				{
					const uint64 l_ui64SamplingFrequency =  ((OpenViBEToolkit::TSignalDecoder < CBoxAlgorithmCSVFileWriter >*)m_pStreamDecoder)->getOutputSamplingRate();
					const uint64 l_ui64TimeOfNthSample = ITimeArithmetics::sampleCountToTime(l_ui64SamplingFrequency, s); // assuming chunk start is 0
					const uint64 l_ui64SampleTime = l_ui64StartTime+l_ui64TimeOfNthSample;

					m_oFileStream << ITimeArithmetics::timeToSeconds(l_ui64SampleTime);
				}
				else if(m_oTypeIdentifier==OV_TypeId_Spectrum) 
				{
					m_oFileStream << ITimeArithmetics::timeToSeconds(l_ui64EndTime);
				}
				for(uint32 c=0; c<l_ui32NumChannels; c++)
				{
					m_oFileStream << m_sSeparator.toASCIIString() << l_pMatrix->getBuffer()[c*l_ui32NumSamples+s];
				}

				if(m_bFirstBuffer)
				{
					if(m_oTypeIdentifier==OV_TypeId_Signal)
					{
						const uint64 l_ui64SamplingFrequency =  ((OpenViBEToolkit::TSignalDecoder < CBoxAlgorithmCSVFileWriter >*)m_pStreamDecoder)->getOutputSamplingRate();

						m_oFileStream << m_sSeparator.toASCIIString() << (uint64)l_ui64SamplingFrequency;

						m_bFirstBuffer=false;
					}
					else if(m_oTypeIdentifier==OV_TypeId_Spectrum)
					{
						const IMatrix* l_pMinMaxFrequencyBand =  ((OpenViBEToolkit::TSpectrumDecoder < CBoxAlgorithmCSVFileWriter >*)m_pStreamDecoder)->getOutputMinMaxFrequencyBands();

						m_oFileStream << m_sSeparator.toASCIIString() << l_pMinMaxFrequencyBand->getBuffer()[s*2+0];
						m_oFileStream << m_sSeparator.toASCIIString() << l_pMinMaxFrequencyBand->getBuffer()[s*2+1];
					}
					else
					{
					}
				}
				else
				{
					if(m_oTypeIdentifier==OV_TypeId_Signal)
					{
						m_oFileStream << m_sSeparator.toASCIIString();
					}
					else if(m_oTypeIdentifier==OV_TypeId_Spectrum)
					{
						m_oFileStream << m_sSeparator.toASCIIString() << m_sSeparator.toASCIIString();
					}
					else
					{
					}
				}

				m_oFileStream << "\n";
			}
			m_ui64SampleCount += l_ui32NumSamples;

			m_bFirstBuffer=false;
		}
		if(m_pStreamDecoder->isEndReceived())
		{
		}
		l_rDynamicBoxContext.markInputAsDeprecated(0, i);
	}

	return true;
}

boolean CBoxAlgorithmCSVFileWriter::process_stimulation(void)
{
	IBoxIO& l_rDynamicBoxContext=this->getDynamicBoxContext();

	for(uint32 i=0; i<l_rDynamicBoxContext.getInputChunkCount(0); i++)
	{
		m_pStreamDecoder->decode(i);
		if(m_pStreamDecoder->isHeaderReceived())
		{
			m_oFileStream << "Time (s)" << m_sSeparator.toASCIIString() << "Identifier" << m_sSeparator.toASCIIString() << "Duration\n";
		}
		if(m_pStreamDecoder->isBufferReceived())
		{
			const IStimulationSet* l_pStimulationSet = ((OpenViBEToolkit::TStimulationDecoder < CBoxAlgorithmCSVFileWriter >*)m_pStreamDecoder)->getOutputStimulationSet();
			for(uint32 j=0; j<l_pStimulationSet->getStimulationCount(); j++)
			{
				m_oFileStream << ITimeArithmetics::timeToSeconds(l_pStimulationSet->getStimulationDate(j))
					<< m_sSeparator.toASCIIString() 
					<< l_pStimulationSet->getStimulationIdentifier(j)
					<< m_sSeparator.toASCIIString() 
					<< ITimeArithmetics::timeToSeconds(l_pStimulationSet->getStimulationDuration(j))
					<< "\n";
			}
		}
		if(m_pStreamDecoder->isEndReceived())
		{
		}
		l_rDynamicBoxContext.markInputAsDeprecated(0, i);
	}

	return true;
}
