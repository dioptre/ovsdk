#include <string>
#include <iostream>

#include <openvibe/ovITimeArithmetics.h>

#include "ovpCBoxAlgorithmOVCSVFileWriter.h"

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::FileIO;

CBoxAlgorithmOVCSVFileWriter::CBoxAlgorithmOVCSVFileWriter(void)
	:
	m_StreamDecoder(nullptr)
	, m_StimulationDecoder(nullptr)
{
}

bool CBoxAlgorithmOVCSVFileWriter::initialize(void)
{
	m_IsFileOpen = false;
	m_Epoch = 0;
	OV_ERROR_UNLESS_KRF(this->getStaticBoxContext().getInputType(0, m_TypeIdentifier),
		"Error while getting input type",
		ErrorType::Internal);
	if (m_TypeIdentifier == OV_TypeId_Signal)
	{
		m_WriterLib = OpenViBE::CSV::createCSVLib();
		m_StreamDecoder = new OpenViBEToolkit::TSignalDecoder < CBoxAlgorithmOVCSVFileWriter >();
		OV_ERROR_UNLESS_KRF(m_StreamDecoder->initialize(*this, 0),
			"Error while stream decoder initialization",
			ErrorType::Internal);
		m_WriterLib->setFormatType(OpenViBE::CSV::EStreamType::Signal);
	}
	else
	{
		OV_ERROR_KRF("Input is a type derived from matrix that the box doesn't recognize", ErrorType::BadInput);
	}

	m_StimulationDecoder = new OpenViBEToolkit::TStimulationDecoder < CBoxAlgorithmOVCSVFileWriter >();
	OV_ERROR_UNLESS_KRF(m_StimulationDecoder->initialize(*this, 1),
		"Error while stimulation decoder initialization",
		ErrorType::Internal);

	const CString filename = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);

	OV_ERROR_UNLESS_KRF(m_WriterLib->openFile(filename.toASCIIString(), OpenViBE::CSV::EFileAccessMode::Write),
		OpenViBE::CSV::ICSVLib::getLogError(m_WriterLib->getLastLogError()).c_str(),
		ErrorType::Internal);

	m_IsHeaderReceived = false;

	return true;
}

bool CBoxAlgorithmOVCSVFileWriter::uninitialize(void)
{
	OV_ERROR_UNLESS_KRF(m_WriterLib->noEventsUntilDate(std::numeric_limits<double>::max()),
		OpenViBE::CSV::ICSVLib::getLogError(m_WriterLib->getLastLogError()).c_str(),
		ErrorType::Internal);
	OV_ERROR_UNLESS_KRF(m_WriterLib->writeAllDataToFile(),
		OpenViBE::CSV::ICSVLib::getLogError(m_WriterLib->getLastLogError()).c_str(),
		ErrorType::Internal);
	if (!m_WriterLib->closeFile())
	{
		OV_FATAL_K(OpenViBE::CSV::ICSVLib::getLogError(m_WriterLib->getLastLogError()).c_str() << ": " << m_WriterLib->getLastErrorString().c_str(), ErrorType::Internal);
	}
	OpenViBE::CSV::releaseCSVLib(m_WriterLib);
	OV_ERROR_UNLESS_KRF(m_StreamDecoder->uninitialize(),
		"Error have been thrown error while stream decoder unitialize",
		ErrorType::Internal);
	OV_ERROR_UNLESS_KRF(m_StimulationDecoder->uninitialize(),
		"Error have been thrown error while stimulation decoder unitialize",
		ErrorType::Internal);
	return true;
}

bool CBoxAlgorithmOVCSVFileWriter::processInput(unsigned int inputIndex)
{
	OV_ERROR_UNLESS_KRF(getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess(),
		"Error while marking algorithm as ready to process",
		ErrorType::Internal);
	return true;
}

bool CBoxAlgorithmOVCSVFileWriter::process(void)
{
	OV_ERROR_UNLESS_KRF(processStreamedMatrix(),
		"Error have been thrown during streamed matrix process",
		ErrorType::Internal);
	OV_ERROR_UNLESS_KRF(processStimulation(),
		"Error have been thrown during stimulation process",
		ErrorType::Internal);

	// write into the library
	if (!m_WriterLib->writeDataToFile())
	{
		OV_ERROR_KRF(OpenViBE::CSV::ICSVLib::getLogError(m_WriterLib->getLastLogError()).c_str(), ErrorType::Internal);
	}

	return true;
}

bool CBoxAlgorithmOVCSVFileWriter::processStreamedMatrix(void)
{
	IBoxIO& dynamicBoxContext = this->getDynamicBoxContext();

	for (unsigned int index = 0; index < dynamicBoxContext.getInputChunkCount(0); index++)
	{
		OV_ERROR_UNLESS_KRF(m_StreamDecoder->decode(index),
			"Failed to decode chunk",
			ErrorType::Internal);
		// represents the properties of the input, no data
		const IMatrix* matrix = static_cast<OpenViBEToolkit::TStreamedMatrixDecoder < CBoxAlgorithmOVCSVFileWriter >*>(m_StreamDecoder)->getOutputMatrix();

		if (m_StreamDecoder->isHeaderReceived())
		{
			OV_ERROR_UNLESS_KRF(!m_IsHeaderReceived,
				"Multiple streamed matrix headers received",
				ErrorType::BadInput);
			m_IsHeaderReceived = true;

			if (m_TypeIdentifier == OV_TypeId_Signal)
			{
				std::vector<std::string> dimensionLabels;
				for (size_t index = 0; index < matrix->getDimensionSize(0); index++)
				{
					dimensionLabels.push_back(matrix->getDimensionLabel(0, index));
				}

				OV_ERROR_UNLESS_KRF(m_WriterLib->setSignalInformation(dimensionLabels, static_cast<unsigned int>((static_cast<OpenViBEToolkit::TSignalDecoder<CBoxAlgorithmOVCSVFileWriter>*>(m_StreamDecoder))->getOutputSamplingRate()), matrix->getDimensionSize(1)),
					OpenViBE::CSV::ICSVLib::getLogError(m_WriterLib->getLastLogError()).c_str(),
					ErrorType::Internal);

				OV_ERROR_UNLESS_KRF(m_WriterLib->writeHeaderToFile(),
					OpenViBE::CSV::ICSVLib::getLogError(m_WriterLib->getLastLogError()).c_str(),
					ErrorType::Internal);
			}
		}

		if (m_StreamDecoder->isBufferReceived())
		{
			const IMatrix* imatrix = static_cast<OpenViBEToolkit::TStreamedMatrixDecoder < CBoxAlgorithmOVCSVFileWriter >*>(m_StreamDecoder)->getOutputMatrix();
			std::vector<double> matrixValues;

			if (m_TypeIdentifier == OV_TypeId_Signal)
			{
				const unsigned long long samplingFrequency = static_cast<OpenViBEToolkit::TSignalDecoder < CBoxAlgorithmOVCSVFileWriter >*>(m_StreamDecoder)->getOutputSamplingRate();
				const unsigned long long chunkStartTime = dynamicBoxContext.getInputChunkStartTime(0, index);
				const unsigned int channelCount = matrix->getDimensionSize(0);
				const unsigned int sampleCount = matrix->getDimensionSize(1);

				double lastTime = 0.0;
				for (unsigned int sampleIndex = 0; sampleIndex < sampleCount; sampleIndex++)
				{
					double startTime = 0.0;
					double endTime = 0.0;

					// get starting and ending time

					unsigned long long timeOfNthSample = ITimeArithmetics::sampleCountToTime(samplingFrequency, sampleIndex); // assuming chunk start is 0
					unsigned long long sampleTime = chunkStartTime + timeOfNthSample;
					startTime = ITimeArithmetics::timeToSeconds(sampleTime);
					unsigned long long timeOfNthAndOneSample = ITimeArithmetics::sampleCountToTime(samplingFrequency, sampleIndex + 1);
					endTime = ITimeArithmetics::timeToSeconds(chunkStartTime + timeOfNthAndOneSample);

					// get matrix values
					for (unsigned int channelIndex = 0; channelIndex < channelCount; channelIndex++)
					{
						matrixValues.push_back(imatrix->getBuffer()[channelIndex*sampleCount + sampleIndex]);
					}

					// add sample to the library
					OV_ERROR_UNLESS_KRF(m_WriterLib->addSample({ startTime, endTime, matrixValues, m_Epoch }),
						OpenViBE::CSV::ICSVLib::getLogError(m_WriterLib->getLastLogError()).c_str(),
						ErrorType::Internal);

					matrixValues.clear();
					lastTime = endTime;
				}
			}

			m_Epoch++;
		}

		OV_ERROR_UNLESS_KRF(dynamicBoxContext.markInputAsDeprecated(0, index),
			"Fail to mark input as deprecated",
			ErrorType::Internal);
	}

	return true;
}

bool CBoxAlgorithmOVCSVFileWriter::processStimulation(void)
{
	IBoxIO& dynamicBoxContext = this->getDynamicBoxContext();

	// add every stimulation received
	for (unsigned int index = 0; index < dynamicBoxContext.getInputChunkCount(1); index++)
	{
		OV_ERROR_UNLESS_KRF(m_StimulationDecoder->decode(index),
			"Failed to decode stimulation chunk",
			ErrorType::Internal);
		if (m_StimulationDecoder->isBufferReceived())
		{
			const IStimulationSet* stimulationSet = m_StimulationDecoder->getOutputStimulationSet();
			// for each stimulation, get its informations

			for (unsigned int stimulationIndex = 0; stimulationIndex < stimulationSet->getStimulationCount(); stimulationIndex++)
			{
				OV_ERROR_UNLESS_KRF(m_WriterLib->addEvent({ stimulationSet->getStimulationIdentifier(stimulationIndex),
					ITimeArithmetics::timeToSeconds(stimulationSet->getStimulationDate(stimulationIndex)),
					ITimeArithmetics::timeToSeconds(stimulationSet->getStimulationDuration(stimulationIndex)) }),
					OpenViBE::CSV::ICSVLib::getLogError(m_WriterLib->getLastLogError()).c_str(),
					ErrorType::Internal);
			}
			// set NoEventUntilDate to prevent time that will be empty of stimulations until the end of the last chunk
			OV_ERROR_UNLESS_KRF(m_WriterLib->noEventsUntilDate(ITimeArithmetics::timeToSeconds(dynamicBoxContext.getInputChunkEndTime(1, (dynamicBoxContext.getInputChunkCount(1) - 1)))),
				OpenViBE::CSV::ICSVLib::getLogError(m_WriterLib->getLastLogError()).c_str(),
				ErrorType::Internal);
		}

		if (!dynamicBoxContext.markInputAsDeprecated(1, index))
		{
			OV_FATAL_K("Fail to mark stimulations input as deprecated",
				ErrorType::Internal);
		}
	}

	return true;
}
