#ifndef __OpenViBEPlugins_BoxAlgorithm_MulticlassClassifierTrainer_H__
#define __OpenViBEPlugins_BoxAlgorithm_MulticlassClassifierTrainer_H__

#include "../ovp_defines.h"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include "ovpCBoxAlgorithmCommonClassifierListener.inl"

#include <map>
#include <vector>
#include <iostream>


#define OVP_ClassId_BoxAlgorithm_MulticlassClassifierTrainer     OpenViBE::CIdentifier(0x128703B2, 0x0E2441F7)
#define OVP_ClassId_BoxAlgorithm_MulticlassClassifierTrainerDesc OpenViBE::CIdentifier(0x0A0A3F0B, 0x17C85690)

#define OVP_BoxAlgorithm_MulticlassClassifierTrainer_CommonSettingsCount 5

namespace OpenViBEPlugins
{
    namespace Classification
    {
        class CBoxAlgorithmMulticlassClassifierTrainer : virtual public OpenViBEToolkit::TBoxAlgorithm < OpenViBE::Plugins::IBoxAlgorithm >
        {
        public:

            virtual void release(void) { delete this; }

            virtual OpenViBE::boolean initialize(void);
            virtual OpenViBE::boolean uninitialize(void);
            virtual OpenViBE::boolean processInput(OpenViBE::uint32 ui32InputIndex);
            virtual OpenViBE::boolean process(void);

            _IsDerivedFromClass_Final_(OpenViBEToolkit::TBoxAlgorithm < OpenViBE::Plugins::IBoxAlgorithm >, OVP_ClassId_BoxAlgorithm_MulticlassClassifierTrainer);

        protected:

            virtual OpenViBE::boolean train(const size_t uiStartIndex, const size_t uiStopIndex);
            virtual OpenViBE::float64 getAccuracy(const size_t uiStartIndex, const size_t uiStopIndex);

        protected:

            std::map < OpenViBE::uint32, OpenViBE::uint32 > m_vFeatureCount;
            std::vector < OpenViBE::uint32 > m_vFeatureVectorIndex;
            std::map < OpenViBE::uint32, OpenViBE::Kernel::IAlgorithmProxy*> m_vFeatureVectorsDecoder;
            OpenViBE::Kernel::IAlgorithmProxy* m_pStimulationsDecoder;
            OpenViBE::Kernel::IAlgorithmProxy* m_pClassifier;
            OpenViBE::uint64 m_ui64TrainStimulation;
            OpenViBE::uint64 m_ui64PartitionCount;

            OpenViBE::Kernel::IAlgorithmProxy* m_pStimulationsEncoder;

            typedef struct
            {
                OpenViBE::CMatrix* m_pFeatureVectorMatrix;
                OpenViBE::uint64 m_ui64StartTime;
                OpenViBE::uint64 m_ui64EndTime;
                OpenViBE::uint32 m_ui32InputIndex;
            } SFeatureVector;

            std::vector < CBoxAlgorithmMulticlassClassifierTrainer::SFeatureVector > m_vFeatureVector;
        };

        class CBoxAlgorithmMulticlassClassifierTrainerDesc : virtual public OpenViBE::Plugins::IBoxAlgorithmDesc
        {
        public:

            virtual void release(void) { }

            virtual OpenViBE::CString getName(void) const                { return OpenViBE::CString("Multiclass classifier trainer"); }
            virtual OpenViBE::CString getAuthorName(void) const          { return OpenViBE::CString("Yann Renard"); }
            virtual OpenViBE::CString getAuthorCompanyName(void) const   { return OpenViBE::CString("INRIA/IRISA"); }
            virtual OpenViBE::CString getShortDescription(void) const    { return OpenViBE::CString("Generic classifier trainer, relying on several box algorithms"); }
            virtual OpenViBE::CString getDetailedDescription(void) const { return OpenViBE::CString("Performs multiple training on the feature vector set leaving a single feature vector each time and tests this feature vector on the trained classifier"); }
            virtual OpenViBE::CString getCategory(void) const            { return OpenViBE::CString("Classification"); }
            virtual OpenViBE::CString getVersion(void) const             { return OpenViBE::CString("0.1"); }
            virtual OpenViBE::CString getStockItemName(void) const       { return OpenViBE::CString("gtk-apply"); }

            virtual OpenViBE::CIdentifier getCreatedClass(void) const    { return OVP_ClassId_BoxAlgorithm_MulticlassClassifierTrainer; }
            virtual OpenViBE::Plugins::IPluginObject* create(void)       { return new OpenViBEPlugins::Classification::CBoxAlgorithmMulticlassClassifierTrainer; }

            virtual OpenViBE::boolean getBoxPrototype(
                OpenViBE::Kernel::IBoxProto& rBoxAlgorithmPrototype) const
            {
                rBoxAlgorithmPrototype.addInput  ("Stimulations",                         OV_TypeId_Stimulations);
                rBoxAlgorithmPrototype.addInput  ("Features for class 1",                 OV_TypeId_FeatureVector);
                rBoxAlgorithmPrototype.addInput  ("Features for class 2",                 OV_TypeId_FeatureVector);

                rBoxAlgorithmPrototype.addOutput ("Train-completed Flag",                 OV_TypeId_Stimulations);

                rBoxAlgorithmPrototype.addSetting("Classifier to use",                    OVTK_TypeId_ClassificationAlgorithm, "");
                rBoxAlgorithmPrototype.addSetting("Filename to save configuration to",    OV_TypeId_Filename,                  "");
                rBoxAlgorithmPrototype.addSetting("Train trigger",                        OV_TypeId_Stimulation,               "OVTK_StimulationId_Train");
                rBoxAlgorithmPrototype.addSetting("Number of partitions for k-fold test", OV_TypeId_Integer,                   "10");
                rBoxAlgorithmPrototype.addSetting("Pairing type",                         OVTK_TypeId_PairingClassification,                   "");

                rBoxAlgorithmPrototype.addFlag   (OpenViBE::Kernel::BoxFlag_CanAddInput);
                return true;
            }

            virtual OpenViBE::Plugins::IBoxListener* createBoxListener(void) const { return new CBoxAlgorithmCommonClassifierListener(OVP_BoxAlgorithm_MulticlassClassifierTrainer_CommonSettingsCount); }
            virtual void releaseBoxListener(OpenViBE::Plugins::IBoxListener* pBoxListener) { delete pBoxListener; }

            _IsDerivedFromClass_Final_(OpenViBE::Plugins::IBoxAlgorithmDesc, OVP_ClassId_BoxAlgorithm_MulticlassClassifierTrainerDesc);
        };
    };
};

#endif // __OpenViBEPlugins_BoxAlgorithm_MulticlassClassifierTrainer_H__
