#ifndef __SamplePlugin_Algorithms_CSpectrumDecoder_H__
#define __SamplePlugin_Algorithms_CSpectrumDecoder_H__

#include "ovpCStreamedMatrixDecoder.h"
#include <iomanip>

#define OVP_ClassId_Algorithm_SpectrumStreamDecoder                                         OpenViBE::CIdentifier(0x128202DB, 0x449FC7A6)
#define OVP_ClassId_Algorithm_SpectrumStreamDecoderDesc                                     OpenViBE::CIdentifier(0x54D18EE8, 0x5DBD913A)
#define OVP_Algorithm_SpectrumStreamDecoder_OutputParameterId_FrequencyAbscissa             OpenViBE::CIdentifier(0x14A572E4, 0x5C405C8E)
#define OVP_Algorithm_SpectrumStreamDecoder_OutputParameterId_SamplingRate                  OpenViBE::CIdentifier(0x68442C12, 0x0D9A46DE)

namespace OpenViBEPlugins
{
	namespace StreamCodecs
	{
		class CSpectrumDecoder : public OpenViBEPlugins::StreamCodecs::CStreamedMatrixDecoder
		{
		public:

			virtual void release(void) { delete this; }

			virtual OpenViBE::boolean initialize(void);
			virtual OpenViBE::boolean uninitialize(void);

			_IsDerivedFromClass_Final_(OpenViBEPlugins::StreamCodecs::CStreamedMatrixDecoder, OVP_ClassId_Algorithm_SpectrumStreamDecoder);

			// ebml callbacks
			virtual EBML::boolean isMasterChild(const EBML::CIdentifier& rIdentifier);
			virtual void openChild(const EBML::CIdentifier& rIdentifier);
			virtual void processChildData(const void* pBuffer, const EBML::uint64 ui64BufferSize);
			virtual void closeChild(void);

		protected:

			OpenViBE::Kernel::TParameterHandler < OpenViBE::IMatrix* > op_pFrequencyAbscissa;
			OpenViBE::Kernel::TParameterHandler < OpenViBE::uint64 > op_pSamplingRate;



		private:

			std::stack<EBML::CIdentifier> m_vNodes;

			OpenViBE::uint32 m_ui32FrequencyBandIndex;

			// Value of the current lower frequency of the band. Only used to read old spectrum format.
			double m_lowerFreq;
		};

		class CSpectrumDecoderDesc : public OpenViBEPlugins::StreamCodecs::CStreamedMatrixDecoderDesc
		{
		public:

			virtual void release(void) { }

			virtual OpenViBE::CString getName(void) const                { return OpenViBE::CString("Spectrum stream decoder"); }
			virtual OpenViBE::CString getAuthorName(void) const          { return OpenViBE::CString("Yann Renard"); }
			virtual OpenViBE::CString getAuthorCompanyName(void) const   { return OpenViBE::CString("INRIA/IRISA"); }
			virtual OpenViBE::CString getShortDescription(void) const    { return OpenViBE::CString("Decodes the Spectrum type streams."); }
			virtual OpenViBE::CString getDetailedDescription(void) const { return OpenViBE::CString(""); }
			virtual OpenViBE::CString getCategory(void) const            { return OpenViBE::CString("Stream codecs/Decoders"); }
			virtual OpenViBE::CString getVersion(void) const             { return OpenViBE::CString("1.0"); }
			virtual OpenViBE::CString getSoftwareComponent(void) const   { return OpenViBE::CString("openvibe-sdk"); }
			virtual OpenViBE::CString getAddedSoftwareVersion(void) const   { return OpenViBE::CString("0.0.0"); }
			virtual OpenViBE::CString getUpdatedSoftwareVersion(void) const { return OpenViBE::CString("0.1.0"); }

			virtual OpenViBE::CIdentifier getCreatedClass(void) const    { return OVP_ClassId_Algorithm_SpectrumStreamDecoder; }
			virtual OpenViBE::Plugins::IPluginObject* create(void)       { return new OpenViBEPlugins::StreamCodecs::CSpectrumDecoder(); }

			virtual OpenViBE::boolean getAlgorithmPrototype(
				OpenViBE::Kernel::IAlgorithmProto& rAlgorithmPrototype) const
			{
				OpenViBEPlugins::StreamCodecs::CStreamedMatrixDecoderDesc::getAlgorithmPrototype(rAlgorithmPrototype);

				rAlgorithmPrototype.addOutputParameter(OVP_Algorithm_SpectrumStreamDecoder_OutputParameterId_FrequencyAbscissa, "Frequency abscissa", OpenViBE::Kernel::ParameterType_Matrix);
				rAlgorithmPrototype.addOutputParameter(OVP_Algorithm_SpectrumStreamDecoder_OutputParameterId_SamplingRate, "Sampling rate", OpenViBE::Kernel::ParameterType_UInteger);

				return true;
			}

			_IsDerivedFromClass_Final_(OpenViBEPlugins::StreamCodecs::CStreamedMatrixDecoderDesc, OVP_ClassId_Algorithm_SpectrumStreamDecoderDesc);
		};
	};
};

#endif // __SamplePlugin_Algorithms_CSpectrumDecoder_H__
