#include "ovpCBoxAlgorithmSignalDecimation.h"

#include <system/ovCMemory.h>

#include <openvibe/ovITimeArithmetics.h>

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::SignalProcessing;

bool CBoxAlgorithmSignalDecimation::initialize(void)
{
	m_pStreamDecoder=NULL;
	m_pStreamEncoder=NULL;

	m_i64DecimationFactor=FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
	OV_ERROR_UNLESS_KRF(
		m_i64DecimationFactor > 1,
		"Invalid decimation factor [" << m_i64DecimationFactor << "] (expected value > 1)",
		OpenViBE::Kernel::ErrorType::BadSetting
	);

	m_pStreamDecoder=&this->getAlgorithmManager().getAlgorithm(this->getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_SignalStreamDecoder));
	m_pStreamDecoder->initialize();

	ip_pMemoryBuffer.initialize(m_pStreamDecoder->getInputParameter(OVP_GD_Algorithm_SignalStreamDecoder_InputParameterId_MemoryBufferToDecode));
	op_pMatrix.initialize(m_pStreamDecoder->getOutputParameter(OVP_GD_Algorithm_SignalStreamDecoder_OutputParameterId_Matrix));
	op_ui64SamplingRate.initialize(m_pStreamDecoder->getOutputParameter(OVP_GD_Algorithm_SignalStreamDecoder_OutputParameterId_SamplingRate));

	m_pStreamEncoder=&this->getAlgorithmManager().getAlgorithm(this->getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_SignalStreamEncoder));
	m_pStreamEncoder->initialize();

	ip_ui64SamplingRate.initialize(m_pStreamEncoder->getInputParameter(OVP_GD_Algorithm_SignalStreamEncoder_InputParameterId_SamplingRate));
	ip_pMatrix.initialize(m_pStreamEncoder->getInputParameter(OVP_GD_Algorithm_SignalStreamEncoder_InputParameterId_Matrix));
	op_pMemoryBuffer.initialize(m_pStreamEncoder->getOutputParameter(OVP_GD_Algorithm_SignalStreamEncoder_OutputParameterId_EncodedMemoryBuffer));

	m_ui32ChannelCount=0;
	m_ui32InputSampleIndex=0;
	m_ui32InputSampleCountPerSentBlock=0;
	m_ui64OutputSamplingFrequency = 0;
	m_ui32OutputSampleIndex=0;
	m_ui32OutputSampleCountPerSentBlock=0;

	m_ui64TotalSampleCount=0;
	m_ui64StartTimeBase = 0;
	m_ui64LastStartTime=0;
	m_ui64LastEndTime = 0;

	return true;
}

bool CBoxAlgorithmSignalDecimation::uninitialize(void)
{
	op_pMemoryBuffer.uninitialize();
	ip_pMatrix.uninitialize();
	ip_ui64SamplingRate.uninitialize();

	if(m_pStreamEncoder)
	{
		m_pStreamEncoder->uninitialize();
		this->getAlgorithmManager().releaseAlgorithm(*m_pStreamEncoder);
		m_pStreamEncoder=NULL;
	}

	op_ui64SamplingRate.uninitialize();
	op_pMatrix.uninitialize();
	ip_pMemoryBuffer.uninitialize();

	if(m_pStreamDecoder)
	{
		m_pStreamDecoder->uninitialize();
		this->getAlgorithmManager().releaseAlgorithm(*m_pStreamDecoder);
		m_pStreamDecoder=NULL;
	}

	return true;
}

bool CBoxAlgorithmSignalDecimation::processInput(uint32_t ui32InputIndex)
{
	getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
	return true;
}

bool CBoxAlgorithmSignalDecimation::process(void)
{
	// IBox& l_rStaticBoxContext=this->getStaticBoxContext();
	IBoxIO& l_rDynamicBoxContext=this->getDynamicBoxContext();

	for(uint32_t i = 0; i < l_rDynamicBoxContext.getInputChunkCount(0); i++)
	{
		ip_pMemoryBuffer=l_rDynamicBoxContext.getInputChunk(0, i);
		op_pMemoryBuffer=l_rDynamicBoxContext.getOutputChunk(0);

		uint64_t l_ui64StartTime=l_rDynamicBoxContext.getInputChunkStartTime(0, i);
		uint64_t l_ui64EndTime=l_rDynamicBoxContext.getInputChunkEndTime(0, i);

		if(l_ui64StartTime!=m_ui64LastEndTime)
		{
			m_ui64StartTimeBase=l_ui64StartTime;
			m_ui32InputSampleIndex=0;
			m_ui32OutputSampleIndex=0;
			m_ui64TotalSampleCount=0;
		}

		m_ui64LastStartTime=l_ui64StartTime;
		m_ui64LastEndTime=l_ui64EndTime;

		m_pStreamDecoder->process();
		if(m_pStreamDecoder->isOutputTriggerActive(OVP_GD_Algorithm_SignalStreamDecoder_OutputTriggerId_ReceivedHeader))
		{
			m_ui32InputSampleIndex=0;
			m_ui32InputSampleCountPerSentBlock=op_pMatrix->getDimensionSize(1);
			m_ui64InputSamplingFrequency=op_ui64SamplingRate;

			OV_ERROR_UNLESS_KRF(
				m_ui64InputSamplingFrequency%m_i64DecimationFactor == 0,
				"Failed to decimate: input sampling frequency [" << m_ui64InputSamplingFrequency << "] not multiple of decimation factor [" << m_i64DecimationFactor << "]",
				OpenViBE::Kernel::ErrorType::BadSetting
			);

			m_ui32OutputSampleIndex=0;
			m_ui32OutputSampleCountPerSentBlock = static_cast<uint32_t>(m_ui32InputSampleCountPerSentBlock/m_i64DecimationFactor);
			m_ui32OutputSampleCountPerSentBlock=(m_ui32OutputSampleCountPerSentBlock?m_ui32OutputSampleCountPerSentBlock:1);
			m_ui64OutputSamplingFrequency=op_ui64SamplingRate/m_i64DecimationFactor;

			OV_ERROR_UNLESS_KRF(
				m_ui64OutputSamplingFrequency != 0,
				"Failed to decimate: output sampling frequency is 0",
				OpenViBE::Kernel::ErrorType::BadOutput
			);

			m_ui32ChannelCount=op_pMatrix->getDimensionSize(0);
			m_ui64TotalSampleCount=0;

			OpenViBEToolkit::Tools::Matrix::copyDescription(*ip_pMatrix, *op_pMatrix);
			ip_pMatrix->setDimensionSize(1, m_ui32OutputSampleCountPerSentBlock);
			ip_ui64SamplingRate=m_ui64OutputSamplingFrequency;
			m_pStreamEncoder->process(OVP_GD_Algorithm_SignalStreamEncoder_InputTriggerId_EncodeHeader);
			OpenViBEToolkit::Tools::Matrix::clearContent(*ip_pMatrix);

			l_rDynamicBoxContext.markOutputAsReadyToSend(0, l_ui64StartTime, l_ui64StartTime); // $$$ supposes we have one node per chunk
		}
		if(m_pStreamDecoder->isOutputTriggerActive(OVP_GD_Algorithm_SignalStreamDecoder_OutputTriggerId_ReceivedBuffer))
		{
			float64* l_pInputBuffer=op_pMatrix->getBuffer();
			float64* l_pOutputBuffer=ip_pMatrix->getBuffer()+m_ui32OutputSampleIndex;

			for(uint32_t j = 0; j < m_ui32InputSampleCountPerSentBlock; j++)
			{
				float64* l_pInputBufferTmp=l_pInputBuffer;
				float64* l_pOutputBufferTmp=l_pOutputBuffer;
				for(uint32_t k = 0; k < m_ui32ChannelCount; k++)
				{
					*l_pOutputBufferTmp+=*l_pInputBufferTmp;
					l_pOutputBufferTmp+=m_ui32OutputSampleCountPerSentBlock;
					l_pInputBufferTmp+=m_ui32InputSampleCountPerSentBlock;
				}

				m_ui32InputSampleIndex++;
				if(m_ui32InputSampleIndex==m_i64DecimationFactor)
				{
					m_ui32InputSampleIndex=0;
					float64* l_pOutputBufferTmp=l_pOutputBuffer;
					for(uint32_t k = 0; k < m_ui32ChannelCount; k++)
					{
						*l_pOutputBufferTmp/=m_i64DecimationFactor;
						l_pOutputBufferTmp+=m_ui32OutputSampleCountPerSentBlock;
					}

					l_pOutputBuffer++;
					m_ui32OutputSampleIndex++;
					if(m_ui32OutputSampleIndex==m_ui32OutputSampleCountPerSentBlock)
					{

						l_pOutputBuffer=ip_pMatrix->getBuffer();
						m_ui32OutputSampleIndex=0;
						m_pStreamEncoder->process(OVP_GD_Algorithm_SignalStreamEncoder_InputTriggerId_EncodeBuffer);
						uint64_t l_ui64SampleStartTime = m_ui64StartTimeBase + ITimeArithmetics::sampleCountToTime(m_ui64OutputSamplingFrequency, m_ui64TotalSampleCount);
						uint64_t l_ui64SampleEndTime   = m_ui64StartTimeBase + ITimeArithmetics::sampleCountToTime(m_ui64OutputSamplingFrequency, m_ui64TotalSampleCount + m_ui32OutputSampleCountPerSentBlock);
						l_rDynamicBoxContext.markOutputAsReadyToSend(0, l_ui64SampleStartTime, l_ui64SampleEndTime);
						m_ui64TotalSampleCount+=m_ui32OutputSampleCountPerSentBlock;

						OpenViBEToolkit::Tools::Matrix::clearContent(*ip_pMatrix);
					}
				}

				l_pInputBuffer++;
			}
		}
		if(m_pStreamDecoder->isOutputTriggerActive(OVP_GD_Algorithm_SignalStreamDecoder_OutputTriggerId_ReceivedEnd))
		{
			m_pStreamEncoder->process(OVP_GD_Algorithm_SignalStreamEncoder_InputTriggerId_EncodeEnd);
			l_rDynamicBoxContext.markOutputAsReadyToSend(0, l_ui64StartTime, l_ui64StartTime); // $$$ supposes we have one node per chunk
		}

		l_rDynamicBoxContext.markInputAsDeprecated(0, i);
	}
	return true;
}
