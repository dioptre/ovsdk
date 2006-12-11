#ifndef __OpenViBEKernel_Kernel_CLink_H__
#define __OpenViBEKernel_Kernel_CLink_H__

#include "ovkTAttributable.h"
#include "ovkTKernelObject.h"

namespace OpenViBE
{
	namespace Kernel
	{
		class CLink : virtual public OpenViBE::Kernel::TKernelObject<OpenViBE::Kernel::TAttributable<OpenViBE::Kernel::ILink> >
		{
		public:

			CLink(const OpenViBE::Kernel::IKernelContext& rKernelContext);

			virtual OpenViBE::boolean setIdentifier(
				const OpenViBE::CIdentifier& rIdentifier);
			virtual const OpenViBE::CIdentifier& getIdentifier(void) const;

			virtual OpenViBE::boolean setSource(
				const OpenViBE::CIdentifier& rBoxIdentifier,
				const OpenViBE::uint32 ui32BoxOutputIndex);
			virtual OpenViBE::boolean setTarget(
				const OpenViBE::CIdentifier& rBoxIdentifier,
				const OpenViBE::uint32 ui32BoxInputIndex);
			virtual OpenViBE::boolean getSource(
				OpenViBE::CIdentifier& rBoxIdentifier,
				OpenViBE::uint32& ui32BoxOutputIndex) const;
			virtual OpenViBE::CIdentifier getSourceBoxIdentifier(void) const;
			virtual OpenViBE::uint32 getSourceBoxOutputIndex(void) const;
			virtual OpenViBE::boolean getTarget(
				OpenViBE::CIdentifier& rTargetBoxIdentifier,
				OpenViBE::uint32& ui32BoxInputIndex) const;
			virtual OpenViBE::CIdentifier getTargetBoxIdentifier(void) const;
			virtual OpenViBE::uint32 getTargetBoxInputIndex(void) const;

			_IsDerivedFromClass_Final_(OpenViBE::Kernel::TKernelObject<OpenViBE::Kernel::TAttributable<OpenViBE::Kernel::ILink> >, OVK_ClassId_Kernel_Link)

		protected:

			OpenViBE::CIdentifier m_oIdentifier;
			OpenViBE::CIdentifier m_oSourceBoxIdentifier;
			OpenViBE::CIdentifier m_oTargetBoxIdentifier;
			OpenViBE::uint32 m_ui32SourceOutputIndex;
			OpenViBE::uint32 m_ui32TargetInputIndex;
		};
	};
};

#endif // __OpenViBEKernel_Kernel_CLink_H__
