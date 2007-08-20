#ifndef __OpenViBEKernel_Kernel_Player_CDynamicBoxContext_H__
#define __OpenViBEKernel_Kernel_Player_CDynamicBoxContext_H__

#include "../ovkTKernelObject.h"

#include "../../tools/ovkTBoxIOProxy.h"

class PsSimulatedBox;

namespace OpenViBE
{
	namespace Kernel
	{
		class CDynamicBoxContext : virtual public OpenViBE::Tools::TBoxIOProxy<OpenViBE::Kernel::TKernelObject<OpenViBE::Kernel::IBoxIO> >
		{
		public:

			CDynamicBoxContext(const OpenViBE::Kernel::IKernelContext& rKernelContext, ::PsSimulatedBox* pSimulatedBox);

			_IsDerivedFromClass_Final_(OpenViBE::Tools::TBoxIOProxy<OpenViBE::Kernel::TKernelObject<OpenViBE::Kernel::IBoxIO> >, OVK_ClassId_Kernel_Player_DynamicBoxContext)
		};
	};
};

#endif // __OpenViBEKernel_Kernel_Player_CDynamicBoxContext_H__
