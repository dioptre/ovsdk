#ifndef ExternalP300StimulatorDefinitions2
#define ExternalP300StimulatorDefinitions2

#ifndef StimulationIdDefinitions
#define StimulationIdDefinitions
	/** */
	#define OVA_StimulationId_ExperimentStart				181
	/** */
	#define OVA_StimulationId_ExperimentStop				182
	/** */
	#define OVA_StimulationId_TrialStart					183
	/** */
	#define OVA_StimulationId_TrialStop						184
	/** not used */
	#define OVA_StimulationId_SegmentStart					185
	/** not used */
	#define OVA_StimulationId_SegmentStop					186
	/** */
	#define OVA_StimulationId_RestStart						187
	/** */
	#define OVA_StimulationId_RestStop						188
	/** not used */
	#define OVA_StimulationId_VisualStimulationStart		189
	/** to reset the display */
	#define OVA_StimulationId_VisualStimulationStop			190
	/** in copy or calibration each flash is either flagged as target or non-target, for ease of training XDAWN and the classifier in OpenViBE*/
	#define OVA_StimulationId_Target						191
	/** in copy or calibration each flash is either flagged as target or non-target, for ease of training XDAWN and the classifier in OpenViBE*/
	#define OVA_StimulationId_NonTarget						192
	/** send to the OpenViBE designer to ask the evidence accumulator to flush the latest prediction and letter probabilities*/
	#define OVA_StimulationId_FeedbackCue					193
	/** indicates a target letter will appear during copy or calibration. The following stimulus should range from 1 to the number of symbols on the keyboard*/
	#define OVA_StimulationId_TargetCue						194
	/** */
	#define OVA_StimulationId_Flash							195
	/** */
	#define OVA_StimulationId_ErpDetected					196
	/** indicates new symbol probabilities are available which the visualiser can use to color certain symbols*/
	#define OVA_StimulationId_LetterColorFeedback			197
	/** */
	#define OVA_StimulationId_FlashStop						198
	/** is send to the designer so that it trains the classifier and XDAWN after a 10-letter copy or calibration spelling with the subject independent classifier*/
	#define OVA_StimulationId_UpdateModel					199
	/** these stimuli range from 1 to the number of symbols in the keyboard and are used to indicate which letter should be the target letter or which letter was predicted*/
	#define OVA_StimulationId_Label(i)                      0+i
#endif

#define GL_GLEXT_PROTOTYPES

//#define OUTPUT_TIMING

/*#ifndef ThreadMessageEnumerations
#define ThreadMessageEnumerations
enum ThreadMessage {
	THRMSG_IDLE = 0,
	THRMSG_INIT = 1,
	THRMSG_STOP = 2,
	THRMSG_DRAW = 3,
};
#endif*/

#ifndef VisualStateEnumerations
#define VisualStateEnumerations
enum VisualState {
	NOFLASH = 0,
	FLASH = 1,
	CENTRAL_FEEDBACK_CORRECT = 2,
	CENTRAL_FEEDBACK_WRONG = 3,
	NONCENTRAL_FEEDBACK_CORRECT = 4,
	NONCENTRAL_FEEDBACK_WRONG = 5,
	TARGET = 6,
	NONCENTRAL_FEEDBACK_WRONG_SELECTED = 7,
	NONE = 8,
};
#endif


#ifdef TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines
 #include "ovp_global_defines.h"
#endif // TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines

//___________________________________________________________________//
//                                                                   //
// Operating System identification                                   //
//___________________________________________________________________//
//                                                                   //

// #define OVP_OS_Linux
// #define OVP_OS_Windows
// #define OVP_OS_MacOS
// #define OVP_OS_

#if defined TARGET_OS_Windows
 #define OVA_OS_Windows
#elif defined TARGET_OS_Linux
 #define OVA_OS_Linux
#elif defined TARGET_OS_MacOS
 #define OVA_OS_MacOS
#else
 #warning "No target operating system defined !"
#endif

//___________________________________________________________________//
//                                                                   //
// Hardware Architecture identification                              //
//___________________________________________________________________//
//                                                                   //

// #define OVP_ARCHITECTURE_i386
// #define OVP_ARCHITECTURE_

#if defined TARGET_ARCHITECTURE_i386
 #define OVA_ARCHITECTURE_i386
#else
 #warning "No target architecture defined !"
#endif

//___________________________________________________________________//
//                                                                   //
// Compilator software identification                                //
//___________________________________________________________________//
//                                                                   //

// #define OVP_COMPILATOR_GCC
// #define OVP_COMPILATOR_VisualStudio
// #define OVP_COMPILATOR_

#if defined TARGET_COMPILER_GCC
 #define OVA_COMPILER_GCC
#elif defined TARGET_COMPILER_VisualStudio
 #define OVA_COMPILER_VisualStudio
#else
 #warning "No target compilator defined !"
#endif

//___________________________________________________________________//
//                                                                   //
// API Definition                                                    //
//___________________________________________________________________//
//                                                                   //

// Taken from
// - http://people.redhat.com/drepper/dsohowto.pdf
// - http://www.nedprod.com/programs/gccvisibility.html
#if defined OVA_Shared
 #if defined OVA_OS_Windows
  #define OVA_API_Export __declspec(dllexport)
  #define OVA_API_Import __declspec(dllimport)
 #elif defined OVA_OS_Linux
  #define OVA_API_Export __attribute__((visibility("default")))
  #define OVA_API_Import __attribute__((visibility("default")))
 #else
  #define OVA_API_Export
  #define OVA_API_Import
 #endif
#else
 #define OVA_API_Export
 #define OVA_API_Import
#endif

#if defined OVA_Exports
 #define OVA_API OVA_API_Export
#else
 #define OVA_API OVP_API_Import
#endif

//___________________________________________________________________//
//                                                                   //
// NULL Definition                                                   //
//___________________________________________________________________//
//                                                                   //

#ifndef NULL
#define NULL 0
#endif

//taken from glut.h to keep from including windows.h and so to keep a clean name space

#ifdef OVA_OS_Windows
	# if 0
	   /* This would put tons of macros and crap in our clean name space. */
	#  define  WIN32_LEAN_AND_MEAN
	#  include <windows.h>
	# else
	   /* XXX This is from Win32's <windef.h> */
	#  ifndef APIENTRY
	#   define GLUT_APIENTRY_DEFINED
	#   if (_MSC_VER >= 800) || defined(_STDCALL_SUPPORTED) || defined(__BORLANDC__) || defined(__LCC__)
	#    define APIENTRY    __stdcall
	#   else
	#    define APIENTRY
	#   endif
	#  endif
	   /* XXX This is from Win32's <winnt.h> */
	#  ifndef CALLBACK
	#   if (defined(_M_MRX000) || defined(_M_IX86) || defined(_M_ALPHA) || defined(_M_PPC)) && !defined(MIDL_PASS) || defined(__LCC__)
	#    define CALLBACK __stdcall
	#   else
	#    define CALLBACK
	#   endif
	#  endif
	   /* XXX Hack for lcc compiler.  It doesn't support __declspec(dllimport), just __stdcall. */
	#  if defined( __LCC__ )
	#   undef WINGDIAPI
	#   define WINGDIAPI __stdcall
	#  else
	   /* XXX This is from Win32's <wingdi.h> and <winnt.h> */
	#   ifndef WINGDIAPI
	#    define GLUT_WINGDIAPI_DEFINED
	#    define WINGDIAPI __declspec(dllimport)
	#   endif
	#  endif
	   /* XXX This is from Win32's <ctype.h> */
	#  ifndef _WCHAR_T_DEFINED
	typedef unsigned short wchar_t;
	#   define _WCHAR_T_DEFINED
	#  endif
	# endif
# endif

#endif