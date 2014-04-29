#ifndef __OpenViBEPlugins_Defines_H__
#define __OpenViBEPlugins_Defines_H__

#define OVP_Classification_BoxTrainerXMLVersion								2

#define OVP_Algorithm_Classifier_InputParameter_ProbabilityMatrix			OpenViBE::CIdentifier(0xF48D35AD, 0xB8EFF834)
#define OVP_Algorithm_Classifier_OutputParameter_ProbabilityVector			OpenViBE::CIdentifier(0x883599FE, 0x2FDB32FF)

#define OVP_Algorithm_Classifier_Pairwise_InputTriggerId_Train				OpenViBE::CIdentifier(0x32219D21, 0xD3BE6105)
#define OVP_Algorithm_Classifier_Pairwise_InputTriggerId_Classifiy			OpenViBE::CIdentifier(0x3637344B, 0x05D03D7E)
#define OVP_Algorithm_Classifier_Pairwise_InputTriggerId_SaveConfiguration	OpenViBE::CIdentifier(0xF19574AD, 0x024045A7)
#define OVP_Algorithm_Classifier_Pairwise_InputTriggerId_LoadConfiguration	OpenViBE::CIdentifier(0x97AF6C6C, 0x670A12E6)

//___________________________________________________________________//
//                                                                   //
// Plugin Object Descriptor Class Identifiers                        //
//___________________________________________________________________//
//                                                                   //
#define OVP_ClassId_LDAClassifierDesc										OpenViBE::CIdentifier(0x1AE009FE, 0xF4FB82FB)


//___________________________________________________________________//
//                                                                   //
// Plugin Object Class Identifiers                                   //
//___________________________________________________________________//
//                                                                   //

#define OVP_ClassId_LDAClassifier											OpenViBE::CIdentifier(0x49F18236, 0x75AE12FD)

//___________________________________________________________________//
//                                                                   //
// Global defines                                                   //
//___________________________________________________________________//
//                                                                   //

#ifdef TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines
 #include "ovp_global_defines.h"
#endif // TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines


#define OVP_TypeId_ClassificationPairwiseStrategy							OpenViBE::CIdentifier(0x0DD51C74, 0x3C4E74C9)


extern const char* const c_sXmlVersionAttributeName;
extern const char* const c_sIdentifierAttributeName;

extern const char* const c_sStrategyNodeName;
extern const char* const c_sAlgorithmNodeName;
extern const char* const c_sStimulationsNodeName;
extern const char* const c_sRejectedClassNodeName;
extern const char* const c_sClassStimulationNodeName;

extern const char* const c_sClassificationBoxRoot;
extern const char* const c_sClassifierRoot;

#endif // __OpenViBEPlugins_Defines_H__
