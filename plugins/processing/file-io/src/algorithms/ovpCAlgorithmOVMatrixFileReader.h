#ifndef __OpenViBEPlugins_Algorithm_OVMatrixFileReader_H__
#define __OpenViBEPlugins_Algorithm_OVMatrixFileReader_H__

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#define OVP_ClassId_Algorithm_OVMatrixFileReader                               OpenViBE::CIdentifier(0x10661A33, 0x0B0F44A7)
#define OVP_ClassId_Algorithm_OVMatrixFileReaderDesc                           OpenViBE::CIdentifier(0x0E873B5E, 0x0A287FCB)


#define OVP_Algorithm_OVMatrixFileReader_InputParameterId_Filename             OpenViBE::CIdentifier(0x28F87B29, 0x0B09737E)
#define OVP_Algorithm_OVMatrixFileReader_OutputParameterId_Matrix              OpenViBE::CIdentifier(0x2F9521E0, 0x027D789F)
#define OVP_Algorithm_OVMatrixFileReader_InputTriggerId_Open                   OpenViBE::CIdentifier(0x2F996376, 0x2A942485)
#define OVP_Algorithm_OVMatrixFileReader_InputTriggerId_Load                   OpenViBE::CIdentifier(0x22841807, 0x102D681C)
#define OVP_Algorithm_OVMatrixFileReader_InputTriggerId_Close                  OpenViBE::CIdentifier(0x7FDE77DA, 0x384A0B3D)
#define OVP_Algorithm_OVMatrixFileReader_OutputTriggerId_Error                 OpenViBE::CIdentifier(0x6D4F2F4B, 0x05EC6CB9)
#define OVP_Algorithm_OVMatrixFileReader_OutputTriggerId_DataProduced          OpenViBE::CIdentifier(0x76F46051, 0x003B6FE8)

namespace OpenViBEPlugins
{
	namespace FileIO
	{
		class CAlgorithmOVMatrixFileReader : public OpenViBEToolkit::TAlgorithm < OpenViBE::Plugins::IAlgorithm >
		{
		public:

			virtual void release(void) { delete this; }

			virtual OpenViBE::boolean initialize(void);
			virtual OpenViBE::boolean uninitialize(void);
			virtual OpenViBE::boolean process(void);

			_IsDerivedFromClass_Final_(OpenViBEToolkit::TAlgorithm < OpenViBE::Plugins::IAlgorithm >, OVP_ClassId_Algorithm_OVMatrixFileReader);

		protected:

			OpenViBE::Kernel::TParameterHandler < OpenViBE::CString* > ip_sFilename;
			OpenViBE::Kernel::TParameterHandler < OpenViBE::IMatrix* > op_pMatrix;
		};

		class CAlgorithmOVMatrixFileReaderDesc : public OpenViBE::Plugins::IAlgorithmDesc
		{
		public:

			virtual void release(void) { }

			virtual OpenViBE::CString getName(void) const                { return OpenViBE::CString("OpenViBE Matrix file reader"); }
			virtual OpenViBE::CString getAuthorName(void) const          { return OpenViBE::CString("Vincent Delannoy"); }
			virtual OpenViBE::CString getAuthorCompanyName(void) const   { return OpenViBE::CString("INRIA/IRISA"); }
			virtual OpenViBE::CString getShortDescription(void) const    { return OpenViBE::CString(""); }
			virtual OpenViBE::CString getDetailedDescription(void) const { return OpenViBE::CString(""); }
			virtual OpenViBE::CString getCategory(void) const            { return OpenViBE::CString("File reading and writing"); }
			virtual OpenViBE::CString getVersion(void) const             { return OpenViBE::CString("1.1"); }

			virtual OpenViBE::CString getSoftwareComponent(void) const   { return OpenViBE::CString("openvibe-sdk"); }
			virtual OpenViBE::CString getAddedSoftwareVersion(void) const   { return OpenViBE::CString("0.0.0"); }
			virtual OpenViBE::CString getUpdatedSoftwareVersion(void) const { return OpenViBE::CString("0.0.0"); }
			virtual OpenViBE::CIdentifier getCreatedClass(void) const    { return OVP_ClassId_Algorithm_OVMatrixFileReader; }
			virtual OpenViBE::Plugins::IPluginObject* create(void)       { return new OpenViBEPlugins::FileIO::CAlgorithmOVMatrixFileReader; }

			virtual OpenViBE::boolean getAlgorithmPrototype(
				OpenViBE::Kernel::IAlgorithmProto& rAlgorithmPrototype) const
			{
				rAlgorithmPrototype.addInputParameter (OVP_Algorithm_OVMatrixFileReader_InputParameterId_Filename, "Filename", OpenViBE::Kernel::ParameterType_String);
				rAlgorithmPrototype.addOutputParameter(OVP_Algorithm_OVMatrixFileReader_OutputParameterId_Matrix, "Matrix", OpenViBE::Kernel::ParameterType_Matrix);
				return true;
			}

			_IsDerivedFromClass_Final_(OpenViBE::Plugins::IAlgorithmDesc, OVP_ClassId_Algorithm_OVMatrixFileReaderDesc);
		};
	};
};

#endif // __OpenViBEPlugins_Algorithm_OVMatrixFileReader_H__
