#include "ovpCAlgorithmClassifierLDA.h"
#if defined TARGET_HAS_ThirdPartyEIGEN

#include <map>
#include <sstream>
#include <iostream>
#include <algorithm>

#include <system/ovCMemory.h>
#include <xml/IXMLHandler.h>

#include <Eigen/Eigenvalues>

#include "../algorithms/ovpCAlgorithmConditionedCovariance.h"

namespace{
	const char* const c_sTypeNodeName = "LDA";
	const char* const c_sClassesNodeName = "Classes";
//	const char* const c_sCoefficientsNodeName = "Weights";
//	const char* const c_sBiasDistanceNodeName = "Bias-distance";
//	const char* const c_sCoefficientProbabilityNodeName = "Coefficient-probability";
	const char* const c_sComputationHelpersConfigurationNode = "Class-config-list";
	const char* const c_sLDAConfigFileVersionAttributeName = "version";
}

extern const char* const c_sClassifierRoot;

OpenViBE::int32 OpenViBEPlugins::Classification::LDAClassificationCompare(OpenViBE::IMatrix& rFirstClassificationValue, OpenViBE::IMatrix& rSecondClassificationValue)
{
	//We first need to find the best classification of each.
	OpenViBE::float64* l_pClassificationValueBuffer = rFirstClassificationValue.getBuffer();
	const OpenViBE::float64 l_f64MaxFirst = *(std::max_element(l_pClassificationValueBuffer, l_pClassificationValueBuffer+rFirstClassificationValue.getBufferElementCount()));

	l_pClassificationValueBuffer = rSecondClassificationValue.getBuffer();
	const OpenViBE::float64 l_f64MaxSecond = *(std::max_element(l_pClassificationValueBuffer, l_pClassificationValueBuffer+rSecondClassificationValue.getBufferElementCount()));

	//Then we just compared them
	if(ov_float_equal(l_f64MaxFirst, l_f64MaxSecond))
	{
		return 0;
	}
	else if(l_f64MaxFirst > l_f64MaxSecond)
	{
		return -1;
	}
	return 1;
}

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

using namespace OpenViBEPlugins::Classification;

using namespace OpenViBEToolkit;

using namespace Eigen;

#define LDA_DEBUG 0
#if LDA_DEBUG
void CAlgorithmClassifierLDA::dumpMatrix(OpenViBE::Kernel::ILogManager &rMgr, const MatrixXdRowMajor &mat, const CString &desc)
{
	rMgr << LogLevel_Info << desc << "\n";
	for(int i=0;i<mat.rows();i++) {
		rMgr << LogLevel_Info << "Row " << i << ": ";
		for(int j=0;j<mat.cols();j++) {
			rMgr << mat(i,j) << " ";
		}
		rMgr << "\n";
	}
}
#else
void CAlgorithmClassifierLDA::dumpMatrix(OpenViBE::Kernel::ILogManager& /* rMgr */, const MatrixXdRowMajor& /*mat*/, const CString& /*desc*/) { }
#endif

uint32 CAlgorithmClassifierLDA::getOutputProbabilityVectorLength()
{
	return m_vDiscriminantFunctions.size();
}

uint32 CAlgorithmClassifierLDA::getOutputDistanceVectorLength()
{
	return m_vDiscriminantFunctions.size();
}


boolean CAlgorithmClassifierLDA::initialize(void)
{
	// Initialize the Conditioned Covariance Matrix algorithm
	m_pCovarianceAlgorithm = &this->getAlgorithmManager().getAlgorithm(this->getAlgorithmManager().createAlgorithm(OVP_ClassId_Algorithm_ConditionedCovariance));

	OV_ERROR_UNLESS_KRF(
		m_pCovarianceAlgorithm->initialize(),
		"Failed to initialize covariance algorithm",
		OpenViBE::Kernel::ErrorType::Internal
	);

	// This is the weight parameter local to this module and automatically exposed to the GUI. Its redirected to the corresponding parameter of the cov alg.
	TParameterHandler< float64 > ip_f64Shrinkage(this->getInputParameter(OVP_Algorithm_ClassifierLDA_InputParameterId_Shrinkage));
	ip_f64Shrinkage.setReferenceTarget(m_pCovarianceAlgorithm->getInputParameter(OVP_Algorithm_ConditionedCovariance_InputParameterId_Shrinkage));

	TParameterHandler < boolean > ip_bDiagonalCov(this->getInputParameter(OVP_Algorithm_ClassifierLDA_InputParameterId_DiagonalCov));
	ip_bDiagonalCov = false;

	TParameterHandler < XML::IXMLNode* > op_pConfiguration(this->getOutputParameter(OVTK_Algorithm_Classifier_OutputParameterId_Configuration));
	op_pConfiguration=NULL;

	return CAlgorithmClassifier::initialize();
}

boolean CAlgorithmClassifierLDA::uninitialize(void)
{
	OV_ERROR_UNLESS_KRF(
		m_pCovarianceAlgorithm->uninitialize(),
		"Failed to uninitialize covariance algorithm",
		OpenViBE::Kernel::ErrorType::Internal
	);

	this->getAlgorithmManager().releaseAlgorithm(*m_pCovarianceAlgorithm);

	return CAlgorithmClassifier::uninitialize();
}

boolean CAlgorithmClassifierLDA::train(const IFeatureVectorSet& rFeatureVectorSet)
{
	OV_ERROR_UNLESS_KRF(
		this->initializeExtraParameterMechanism(),
		"Failed to unitialize extra parameters",
		OpenViBE::Kernel::ErrorType::Internal
	);

	//We need to clear list because a instance of this class should support more that one training.
	m_vLabelList.clear();
	m_vDiscriminantFunctions.clear();

	const boolean l_bUseShrinkage = this->getBooleanParameter(OVP_Algorithm_ClassifierLDA_InputParameterId_UseShrinkage);

	boolean l_pDiagonalCov;
	if(l_bUseShrinkage)
	{
		this->getFloat64Parameter(OVP_Algorithm_ClassifierLDA_InputParameterId_Shrinkage);
		l_pDiagonalCov = this->getBooleanParameter(OVP_Algorithm_ClassifierLDA_InputParameterId_DiagonalCov);

	}
	else
	{
		//If we don't use shrinkage we need to set lambda to 0.
		TParameterHandler< float64 > ip_f64Shrinkage(this->getInputParameter(OVP_Algorithm_ClassifierLDA_InputParameterId_Shrinkage));
		ip_f64Shrinkage = 0.0;

		TParameterHandler < boolean > ip_bDiagonalCov(this->getInputParameter(OVP_Algorithm_ClassifierLDA_InputParameterId_DiagonalCov));
		ip_bDiagonalCov = false;
		l_pDiagonalCov = false;
	}

	OV_ERROR_UNLESS_KRF(
		this->uninitializeExtraParameterMechanism(),
		"Failed to ininitialize extra parameters",
		OpenViBE::Kernel::ErrorType::Internal
	);

	// IO to the covariance alg
	TParameterHandler < OpenViBE::IMatrix* > op_pMean(m_pCovarianceAlgorithm->getOutputParameter(OVP_Algorithm_ConditionedCovariance_OutputParameterId_Mean));
	TParameterHandler < OpenViBE::IMatrix* > op_pCovarianceMatrix(m_pCovarianceAlgorithm->getOutputParameter(OVP_Algorithm_ConditionedCovariance_OutputParameterId_CovarianceMatrix));
	TParameterHandler < OpenViBE::IMatrix* > ip_pFeatureVectorSet(m_pCovarianceAlgorithm->getInputParameter(OVP_Algorithm_ConditionedCovariance_InputParameterId_FeatureVectorSet));

	const uint32 l_ui32nRows = rFeatureVectorSet.getFeatureVectorCount();
	const uint32 l_ui32nCols = (l_ui32nRows > 0 ? rFeatureVectorSet[0].getSize() : 0);
	this->getLogManager() << LogLevel_Debug << "Feature set input dims ["
		<< rFeatureVectorSet.getFeatureVectorCount() << "x" << l_ui32nCols << "]\n";

	OV_ERROR_UNLESS_KRF(
		l_ui32nRows != 0 && l_ui32nCols != 0,
		"Input data has a zero-size dimension, dims = [" << l_ui32nRows << "x" << l_ui32nCols << "]",
		OpenViBE::Kernel::ErrorType::BadInput
	);

	// The max amount of classes to be expected
	TParameterHandler < uint64 > ip_pNumberOfClasses(this->getInputParameter(OVTK_Algorithm_Classifier_InputParameterId_NumberOfClasses));
	m_ui32NumClasses = static_cast<uint32>(ip_pNumberOfClasses);

	// Count the classes actually present
	std::vector< uint32 > l_vClassCounts;
	l_vClassCounts.resize(m_ui32NumClasses);

	for(uint32 i=0; i<rFeatureVectorSet.getFeatureVectorCount(); i++)
	{
		uint32 classIdx = static_cast<uint32>(rFeatureVectorSet[i].getLabel());
		l_vClassCounts[classIdx]++;
	}

	// Get class labels
	for(uint32 i=0;i<m_ui32NumClasses;i++)
	{
		m_vLabelList.push_back(i);
		m_vDiscriminantFunctions.push_back(CAlgorithmLDADiscriminantFunction());
	}

	// Per-class means and a global covariance are used to form the LDA model
	MatrixXd* l_oPerClassMeans = new MatrixXd[m_ui32NumClasses];
	MatrixXd l_oGlobalCov = MatrixXd::Zero(l_ui32nCols,l_ui32nCols);

	// We need the means per class
	for(uint32 l_ui32classIdx=0;l_ui32classIdx<m_ui32NumClasses;l_ui32classIdx++)
	{
		if(l_vClassCounts[l_ui32classIdx]>0)
		{
			// const float64 l_f64Label = m_vLabelList[l_ui32classIdx];
			const uint32 l_ui32nExamplesInClass = l_vClassCounts[l_ui32classIdx];

			// Copy all the data of the class to a matrix
			CMatrix l_oClassData;
			l_oClassData.setDimensionCount(2);
			l_oClassData.setDimensionSize(0, l_ui32nExamplesInClass);
			l_oClassData.setDimensionSize(1, l_ui32nCols);
			float64 *l_pBuffer = l_oClassData.getBuffer();
			for(uint32 i=0;i<l_ui32nRows;i++)
			{
				if(rFeatureVectorSet[i].getLabel() == l_ui32classIdx)
				{
					System::Memory::copy(l_pBuffer, rFeatureVectorSet[i].getBuffer(), l_ui32nCols*sizeof(float64));
					l_pBuffer += l_ui32nCols;
				}
			}

			// Get the mean out of it
			Map<MatrixXdRowMajor> l_oDataMapper(l_oClassData.getBuffer(), l_ui32nExamplesInClass, l_ui32nCols);
			const MatrixXd l_oClassMean = l_oDataMapper.colwise().mean().transpose();
			l_oPerClassMeans[l_ui32classIdx] = l_oClassMean;
		}
		else
		{
			MatrixXd l_oTmp; l_oTmp.resize(l_ui32nCols, 1); l_oTmp.setZero();
			l_oPerClassMeans[l_ui32classIdx] = l_oTmp;
		}
	}

	// We need a global covariance, use the regularized cov algorithm
	{
		ip_pFeatureVectorSet->setDimensionCount(2);
		ip_pFeatureVectorSet->setDimensionSize(0, l_ui32nRows);
		ip_pFeatureVectorSet->setDimensionSize(1, l_ui32nCols);
		float64 *l_pBuffer = ip_pFeatureVectorSet->getBuffer();

		// Insert all data as the input of the cov algorithm
		for(uint32 i=0;i<l_ui32nRows;i++)
		{
			System::Memory::copy(l_pBuffer, rFeatureVectorSet[i].getBuffer(), l_ui32nCols*sizeof(float64));
			l_pBuffer += l_ui32nCols;
		}

		// Compute cov
		if(!m_pCovarianceAlgorithm->process()) {

			//Free memory before leaving
			delete[] l_oPerClassMeans;

			OV_ERROR_KRF("Global covariance computation failed", OpenViBE::Kernel::ErrorType::Internal);
		}

		// Get the results from the cov algorithm
		Map<MatrixXdRowMajor> l_oCovMapper(op_pCovarianceMatrix->getBuffer(), l_ui32nCols, l_ui32nCols);
		l_oGlobalCov = l_oCovMapper;
	}

	//dumpMatrix(this->getLogManager(), l_aMean[l_ui32classIdx], "Mean");
	//dumpMatrix(this->getLogManager(), l_oGlobalCov, "Shrinked cov");

	if(l_pDiagonalCov)
	{
		for(uint32 i=0;i<l_ui32nCols;i++)
		{
			for(uint32 j=i+1;j<l_ui32nCols;j++)
			{
				l_oGlobalCov(i,j) = 0.0;
				l_oGlobalCov(j,i) = 0.0;
			}
		}
	}

	// Get the pseudoinverse of the global cov using eigen decomposition for self-adjoint matrices
	const float64 l_f64Tolerance = 1e-10;
	SelfAdjointEigenSolver<MatrixXd> l_oEigenSolver;
	l_oEigenSolver.compute(l_oGlobalCov);
	VectorXd l_oEigenValues = l_oEigenSolver.eigenvalues();
	for(uint32 i=0;i<l_ui32nCols;i++) {
		if(l_oEigenValues(i) >= l_f64Tolerance) {
			l_oEigenValues(i) = 1.0/l_oEigenValues(i);
		}
	}
	const MatrixXd l_oGlobalCovInv = l_oEigenSolver.eigenvectors() * l_oEigenValues.asDiagonal() * l_oEigenSolver.eigenvectors().inverse();

	// const MatrixXd l_oGlobalCovInv = l_oGlobalCov.inverse();
	//We send the bias and the weight of each class to ComputationHelper
	for(size_t i = 0 ; i < getClassCount() ; ++i)
	{
		const float64 l_f64ExamplesInClass = l_vClassCounts[i];
		if(l_f64ExamplesInClass > 0)
		{
			const uint32 l_ui32TotalExamples = rFeatureVectorSet.getFeatureVectorCount();

			// This formula e.g. in Hastie, Tibshirani & Friedman: "Elements...", 2nd ed., p. 109
			const VectorXd l_oWeight = (l_oGlobalCovInv * l_oPerClassMeans[i]);
			const MatrixXd l_oInter = -0.5 * l_oPerClassMeans[i].transpose() * l_oGlobalCovInv * l_oPerClassMeans[i];
			const float64 l_f64Bias = l_oInter(0,0) + std::log(l_f64ExamplesInClass/l_ui32TotalExamples);

			this->getLogManager() << LogLevel_Debug << "Bias for " << static_cast<const OpenViBE::uint64>(i) << " is " << l_f64Bias << ", from " << l_f64ExamplesInClass / l_ui32TotalExamples
		 		<< ", " << l_f64ExamplesInClass << "/" << l_ui32TotalExamples << ", int=" << l_oInter(0,0)
		 		<< "\n";
			// dumpMatrix(this->getLogManager(), l_oPerClassMeans[i], "Means");

			m_vDiscriminantFunctions[i].setWeight(l_oWeight);
			m_vDiscriminantFunctions[i].setBias(l_f64Bias);
		}
		else
		{
			this->getLogManager() << LogLevel_Debug << "Class " << static_cast<const OpenViBE::uint64>(i) << " has no examples\n";
		}
	}

	// Hack for classes with zero examples, give them valid models but such that will always lose
	uint32 l_ui32NonZeroClassIdx=0;
	for(size_t i = 0; i < getClassCount() ; i++)
	{
		if(l_vClassCounts[i]>0)
		{
			l_ui32NonZeroClassIdx = i;
			break;
		}
	}
	for(size_t i = 0; i < getClassCount() ; i++)
	{
		if(l_vClassCounts[i]==0)
		{
			m_vDiscriminantFunctions[i].setWeight(m_vDiscriminantFunctions[l_ui32NonZeroClassIdx].getWeight());
			m_vDiscriminantFunctions[i].setBias(m_vDiscriminantFunctions[l_ui32NonZeroClassIdx].getBias() - 1.0); // Will always lose to the orig
		}
	}

	m_ui32NumCols = l_ui32nCols;

	// Debug output
	//dumpMatrix(this->getLogManager(), l_oGlobalCov, "Global cov");
	//dumpMatrix(this->getLogManager(), l_oEigenValues, "Eigenvalues");
	//dumpMatrix(this->getLogManager(), l_oEigenSolver.eigenvectors(), "Eigenvectors");
	//dumpMatrix(this->getLogManager(), l_oGlobalCovInv, "Global cov inverse");
	//dumpMatrix(this->getLogManager(), m_oCoefficients, "Hyperplane weights");

	delete[] l_oPerClassMeans;
	return true;
}

boolean CAlgorithmClassifierLDA::classify(const IFeatureVector& rFeatureVector, float64& rf64Class, IVector& rClassificationValues, IVector& rProbabilityValue)
{
	OV_ERROR_UNLESS_KRF(
		!m_vDiscriminantFunctions.empty(),
		"LDA discriminant function list is empty",
		OpenViBE::Kernel::ErrorType::BadConfig
	);

	OV_ERROR_UNLESS_KRF(
		rFeatureVector.getSize() == m_vDiscriminantFunctions[0].getWeightVectorSize(),
		"Classifier expected " << m_vDiscriminantFunctions[0].getWeightVectorSize() << " features, got " << rFeatureVector.getSize(),
		OpenViBE::Kernel::ErrorType::BadInput
	);

	const Map<VectorXd> l_oFeatureVec(const_cast<float64*>(rFeatureVector.getBuffer()), rFeatureVector.getSize());
	const VectorXd l_oWeights = l_oFeatureVec;
	const uint32 l_ui32ClassCount = getClassCount();

	float64 *l_pValueArray = new float64[l_ui32ClassCount];
	float64 *l_pProbabilityValue = new float64[l_ui32ClassCount];
	//We ask for all computation helper to give the corresponding class value
	for(size_t i = 0; i < l_ui32ClassCount ; ++i)
	{
		l_pValueArray[i] = m_vDiscriminantFunctions[i].getValue(l_oWeights);
	}

	//p(Ck | x) = exp(ak) / sum[j](exp (aj))
	// with aj = (Weight for class j).transpose() * x + (Bias for class j)

	//Exponential can lead to nan results, so we reduce the computation and instead compute
	// p(Ck | x) = 1 / sum[j](exp(aj - ak))

	//All ak are given by computation helper
	errno = 0;
	for(size_t i = 0 ; i < l_ui32ClassCount ; ++i)
	{
		float64 l_f64ExpSum = 0.;
		for(size_t j = 0 ; j < l_ui32ClassCount ; ++j)
		{
			l_f64ExpSum += exp(l_pValueArray[j] - l_pValueArray[i]);
		}
		l_pProbabilityValue[i] = 1/l_f64ExpSum;
		// std::cout << "p " << i << " = " << l_pProbabilityValue[i] << ", v=" << l_pValueArray[i] << ", " << errno << "\n";
	}

	//Then we just find the highest probability and take it as a result
	uint32 l_ui32ClassIndex = std::distance(l_pValueArray, std::max_element(l_pValueArray, l_pValueArray+l_ui32ClassCount));

	rClassificationValues.setSize(l_ui32ClassCount);
	rProbabilityValue.setSize(l_ui32ClassCount);

	for(size_t i = 0 ; i < l_ui32ClassCount ; ++i)
	{
		rClassificationValues[i] = l_pValueArray[i];
		rProbabilityValue[i] = l_pProbabilityValue[i];
	}

	rf64Class = m_vLabelList[l_ui32ClassIndex];

	delete[] l_pValueArray;
	delete[] l_pProbabilityValue;

	return true;
}

uint32 CAlgorithmClassifierLDA::getClassCount()
{
	return m_ui32NumClasses;
}

XML::IXMLNode* CAlgorithmClassifierLDA::saveConfiguration(void)
{
	XML::IXMLNode *l_pAlgorithmNode  = XML::createNode(c_sTypeNodeName);
	l_pAlgorithmNode->addAttribute(c_sLDAConfigFileVersionAttributeName, "1");

	// Write the classifier to an .xml
	std::stringstream l_sClasses;

	for(size_t i = 0; i< getClassCount() ; ++i)
	{
		l_sClasses << m_vLabelList[i] << " ";
	}

	//Only new version should be recorded so we don't need to test
	XML::IXMLNode *l_pHelpersConfiguration = XML::createNode(c_sComputationHelpersConfigurationNode);
	for(size_t i = 0; i < m_vDiscriminantFunctions.size() ; ++i)
	{
		l_pHelpersConfiguration->addChild(m_vDiscriminantFunctions[i].getConfiguration());
	}

	XML::IXMLNode *l_pTempNode = XML::createNode(c_sClassesNodeName);
	l_pTempNode->setPCData(l_sClasses.str().c_str());
	l_pAlgorithmNode->addChild(l_pTempNode);
	l_pAlgorithmNode->addChild(l_pHelpersConfiguration);

	return l_pAlgorithmNode;
}


//Extract a float64 from the PCDATA of a node
float64 getFloatFromNode(XML::IXMLNode *pNode)
{
	std::stringstream l_sData(pNode->getPCData());
	float64 res;
	l_sData >> res;

	return res;
}

boolean CAlgorithmClassifierLDA::loadConfiguration(XML::IXMLNode *pConfigurationNode)
{
	OV_ERROR_UNLESS_KRF(
		pConfigurationNode->hasAttribute(c_sLDAConfigFileVersionAttributeName),
		 "Invalid model: model trained with an obsolete version of LDA",
		OpenViBE::Kernel::ErrorType::BadConfig
	);

	m_vLabelList.clear();
	m_vDiscriminantFunctions.clear();

	XML::IXMLNode* l_pTempNode = pConfigurationNode->getChildByName(c_sClassesNodeName);

	OV_ERROR_UNLESS_KRF(
		l_pTempNode != NULL,
		 "Failed to retrieve xml node",
		OpenViBE::Kernel::ErrorType::BadParsing
	);

	loadClassesFromNode(l_pTempNode);


	//We send corresponding data to the computation helper
	XML::IXMLNode* l_pConfigsNode = pConfigurationNode->getChildByName(c_sComputationHelpersConfigurationNode);

	for(size_t i = 0 ; i < l_pConfigsNode->getChildCount() ; ++i)
	{
		m_vDiscriminantFunctions.push_back(CAlgorithmLDADiscriminantFunction());
		m_vDiscriminantFunctions[i].loadConfiguration(l_pConfigsNode->getChild(i));
	}

	return true;
}

void CAlgorithmClassifierLDA::loadClassesFromNode(XML::IXMLNode *pNode)
{
	std::stringstream l_sData(pNode->getPCData());
	float64 l_f64Temp;
	while(l_sData >> l_f64Temp)
	{
		m_vLabelList.push_back(l_f64Temp);
	}
	m_ui32NumClasses = m_vLabelList.size();
}

//Load the weight vector
void CAlgorithmClassifierLDA::loadCoefficientsFromNode(XML::IXMLNode *pNode)
{
	std::stringstream l_sData(pNode->getPCData());

	std::vector < float64 > l_vCoefficients;
	float64 l_f64Value;
	while(l_sData >> l_f64Value)
	{
		l_vCoefficients.push_back(l_f64Value);
	}

	m_oWeights.resize(1,l_vCoefficients.size());
	m_ui32NumCols  = l_vCoefficients.size();
	for(size_t i=0; i<l_vCoefficients.size(); i++)
	{
		m_oWeights(0,i)=l_vCoefficients[i];
	}
}

#endif // TARGET_HAS_ThirdPartyEIGEN
