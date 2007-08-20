#include "ovkCPlayerContext.h"
#include "ovkPsSimulatedBox.h"

#include "../../tools/ovk_bridge_bind_function.h"

namespace OpenViBE
{
	namespace Kernel
	{
		namespace
		{
			class CLogManagerBridge : virtual public TKernelObject<ILogManager>
			{
			public:

				CLogManagerBridge(const IKernelContext& rKernelContext, ::PsSimulatedBox* pSimulatedBox) : TKernelObject<ILogManager>(rKernelContext), m_pSimulatedBox(pSimulatedBox) { }

				virtual __BridgeBindFunc1__(getKernelContext().getLogManager(), void, log, , const uint64, ui64Value)
				virtual __BridgeBindFunc1__(getKernelContext().getLogManager(), void, log, , const uint32, ui32Value)
				virtual __BridgeBindFunc1__(getKernelContext().getLogManager(), void, log, , const uint16, ui16Value)
				virtual __BridgeBindFunc1__(getKernelContext().getLogManager(), void, log, , const uint8, ui8Value)

				virtual __BridgeBindFunc1__(getKernelContext().getLogManager(), void, log, , const int64, i64Value)
				virtual __BridgeBindFunc1__(getKernelContext().getLogManager(), void, log, , const int32, i32Value)
				virtual __BridgeBindFunc1__(getKernelContext().getLogManager(), void, log, , const int16, i16Value)
				virtual __BridgeBindFunc1__(getKernelContext().getLogManager(), void, log, , const int8, i8Value)

				virtual __BridgeBindFunc1__(getKernelContext().getLogManager(), void, log, , const float32, f32Value)
				virtual __BridgeBindFunc1__(getKernelContext().getLogManager(), void, log, , const float64, f64Value)

				virtual __BridgeBindFunc1__(getKernelContext().getLogManager(), void, log, , const CIdentifier&, rValue)
				virtual __BridgeBindFunc1__(getKernelContext().getLogManager(), void, log, , const CString&, rValue);
				virtual __BridgeBindFunc1__(getKernelContext().getLogManager(), void, log, , const char*, rValue);

				// virtual __BridgeBindFunc1__(getKernelContext().getLogManager(), void, log, , const ELogLevel, eLogLevel)
				virtual __BridgeBindFunc1__(getKernelContext().getLogManager(), void, log, , const ELogColor, eLogColor)

				virtual __BridgeBindFunc1__(getKernelContext().getLogManager(), boolean, addListener, , ILogListener*, pListener)
				virtual __BridgeBindFunc1__(getKernelContext().getLogManager(), boolean, removeListener, , ILogListener*, pListener)

				virtual __BridgeBindFunc1__(getKernelContext().getLogManager(), boolean, isActive, , ELogLevel, eLogLevel)
				virtual __BridgeBindFunc2__(getKernelContext().getLogManager(), boolean, activate, , ELogLevel, eLogLevel, boolean, bActive)

				virtual void log(const ELogLevel eLogLevel)
				{
					getKernelContext().getLogManager()
						<< eLogLevel
						<< "<"
						<< LogColor_PushStateBit
						<< LogColor_ForegroundBlue
						<< "Box algorithm"
						<< LogColor_PopStateBit
						<< "::"
						<< m_pSimulatedBox->getOVName()
						<< "> ";
				}

				_IsDerivedFromClass_Final_(TKernelObject<ILogManager>, OV_UndefinedIdentifier);

			protected:

				::PsSimulatedBox* m_pSimulatedBox;
			};

			class CScenarioManagerBridge : virtual public TKernelObject<IScenarioManager>
			{
			public:

				CScenarioManagerBridge(const IKernelContext& rKernelContext, ::PsSimulatedBox* pSimulatedBox) : TKernelObject<IScenarioManager>(rKernelContext), m_pSimulatedBox(pSimulatedBox) { }

				virtual __BridgeBindFunc1__(getKernelContext().getScenarioManager(), boolean, createScenario, , CIdentifier&, rScenarioIdentifier)
				virtual __BridgeBindFunc1__(getKernelContext().getScenarioManager(), boolean, releaseScenario, , const CIdentifier&, rScenarioIdentifier)
				virtual __BridgeBindFunc1__(getKernelContext().getScenarioManager(), IScenario&, getScenario, , const CIdentifier&, rScenarioIdentifier)
				virtual __BridgeBindFunc1__(getKernelContext().getScenarioManager(), boolean, enumerateScenarios, const, IScenarioManager::IScenarioEnum&, rCallBack)

				_IsDerivedFromClass_Final_(TKernelObject<IScenarioManager>, OV_UndefinedIdentifier);

			protected:

				::PsSimulatedBox* m_pSimulatedBox;
			};
		};
	};
};

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
#define boolean OpenViBE::boolean

CPlayerContext::CPlayerContext(const IKernelContext& rKernelContext, ::PsSimulatedBox* pSimulatedBox)
	:TKernelObject<IPlayerContext>(rKernelContext)
	,m_pSimulatedBox(pSimulatedBox)
	,m_pLogManagerBridge(NULL)
	,m_pScenarioManagerBridge(NULL)
{
	m_pLogManagerBridge=new CLogManagerBridge(rKernelContext, pSimulatedBox);
	m_pScenarioManagerBridge=new CScenarioManagerBridge(rKernelContext, pSimulatedBox);
}

CPlayerContext::~CPlayerContext(void)
{
	delete m_pScenarioManagerBridge;
	delete m_pLogManagerBridge;
}

boolean CPlayerContext::sendSignal(
	const CMessageSignal& rMessageSignal)
{
	// TODO
	log() << LogLevel_Debug << "CPlayerContext::sendSignal - Not yet implemented\n";
	return false;
}

boolean CPlayerContext::sendMessage(
	const CMessageEvent& rMessageEvent,
	const CIdentifier& rTargetIdentifier)
{
	// TODO
	log() << LogLevel_Debug << "CPlayerContext::sendMessage - Not yet implemented\n";
	return false;
}

boolean CPlayerContext::sendMessage(
	const CMessageEvent& rMessageEvent,
	const CIdentifier* pTargetIdentifier,
	const uint32 ui32TargetIdentifierCount)
{
	// TODO
	log() << LogLevel_Debug << "CPlayerContext::sendMessage - Not yet implemented\n";
	return false;
}

ILogManager& CPlayerContext::getLogManager(void)
{
	return *m_pLogManagerBridge;
}

IScenarioManager& CPlayerContext::getScenarioManager(void)
{
	return *m_pScenarioManagerBridge;
}
