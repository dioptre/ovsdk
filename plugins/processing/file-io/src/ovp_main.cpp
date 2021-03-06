#include "ovp_defines.h"

#include "algorithms/ovpCAlgorithmOVMatrixFileReader.h"
#include "algorithms/ovpCAlgorithmOVMatrixFileWriter.h"

#include "algorithms/xml-scenario/ovpCAlgorithmXMLScenarioExporter.h"
#include "algorithms/xml-scenario/ovpCAlgorithmXMLScenarioImporter.h"


//#include "box-algorithms/csv/ovpCBoxAlgorithmCSVFileWriter.h"
//#include "box-algorithms/csv/ovpCBoxAlgorithmCSVFileReader.h"

#include "box-algorithms/openvibe/ovpCBoxAlgorithmGenericStreamReader.h"
#include "box-algorithms/openvibe/ovpCBoxAlgorithmGenericStreamWriter.h"

#include "box-algorithms/ovpCBoxAlgorithmElectrodeLocalizationFileReader.h"

#include "box-algorithms/csv/ovpCBoxAlgorithmOVCSVFileWriter.h"
#include "box-algorithms/csv/ovpCBoxAlgorithmOVCSVFileReader.h"

OVP_Declare_Begin()

	OVP_Declare_New(OpenViBEPlugins::FileIO::CAlgorithmOVMatrixFileReaderDesc)
	OVP_Declare_New(OpenViBEPlugins::FileIO::CAlgorithmOVMatrixFileWriterDesc)

	OVP_Declare_New(OpenViBEPlugins::FileIO::CAlgorithmXMLScenarioExporterDesc)
	OVP_Declare_New(OpenViBEPlugins::FileIO::CAlgorithmXMLScenarioImporterDesc)

//	OVP_Declare_New(OpenViBEPlugins::FileIO::CBoxAlgorithmCSVFileWriterDesc)
//	OVP_Declare_New(OpenViBEPlugins::FileIO::CBoxAlgorithmCSVFileReaderDesc)

	OVP_Declare_New(OpenViBEPlugins::FileIO::CBoxAlgorithmGenericStreamReaderDesc)
	OVP_Declare_New(OpenViBEPlugins::FileIO::CBoxAlgorithmGenericStreamWriterDesc)

	OVP_Declare_New(OpenViBEPlugins::FileIO::CBoxAlgorithmElectrodeLocalisationFileReaderDesc)

	OVP_Declare_New(OpenViBEPlugins::FileIO::CBoxAlgorithmOVCSVFileWriterDesc)
	OVP_Declare_New(OpenViBEPlugins::FileIO::CBoxAlgorithmOVCSVFileReaderDesc)

	rPluginModuleContext.getScenarioManager().registerScenarioImporter(OV_ScenarioImportContext_SchedulerMetaboxImport, ".mxb", OVP_ClassId_Algorithm_XMLScenarioImporter);
	rPluginModuleContext.getConfigurationManager().createConfigurationToken("ScenarioFileNameExtension.xml", "OpenViBE XML Scenario");
	rPluginModuleContext.getConfigurationManager().createConfigurationToken("ScenarioFileNameExtension.mxs", "Mensia XML Scenario");
	rPluginModuleContext.getConfigurationManager().createConfigurationToken("ScenarioFileNameExtension.mxb", "Mensia XML Component");

OVP_Declare_End()
