// /include/task_manager/detail/nt_types.hpp
#pragma once

namespace task_manager::detail::nt {

enum class PROCESSINFOCLASS : unsigned int {
	ProcessBasicInformation           = 0,
	ProcessQuotaLimits                = 1,
	ProcessIoCounters                 = 2,
	ProcessVmCounters                 = 3,
	ProcessTimes                      = 4,
	ProcessBasePriority               = 5,
	ProcessRaisePriority              = 6,
	ProcessDebugPort                  = 7,
	ProcessExceptionPort              = 8,
	ProcessAccessToken                = 9,
	ProcessLdrInformation             = 10,
	ProcessLdtSize                    = 11,
	ProcessDefaultHardErrorMode       = 12,
	ProcessIoPortHandlers             = 13,
	ProcessPooledUsageAndLimits       = 14,
	ProcessWorkingSetWatch            = 15,
	ProcessUserModeIOPL               = 16,
	ProcessEnableAlignmentFaultFixup  = 17,
	ProcessPriorityClass              = 18,
	ProcessWx86Information            = 19,
	ProcessHandleCount                = 20,
	ProcessAffinityMask               = 21,
	ProcessPriorityBoost              = 22,
	ProcessDeviceMap                  = 23,
	ProcessSessionInformation         = 24,
	ProcessForegroundInformation      = 25,
	ProcessWow64Information           = 26,
	ProcessImageFileName              = 27,
	ProcessLUIDDeviceMapsEnabled      = 28,
	ProcessBreakOnTermination         = 29,
	ProcessDebugObjectHandle          = 30,
	ProcessDebugFlags                 = 31,
	ProcessHandleTracing              = 32,
	ProcessIoPriority                 = 33,
	ProcessExecuteFlags               = 34,
	ProcessTlsInformation             = 35,
	ProcessCookie                     = 36,
	ProcessImageInformation           = 37,
	ProcessCycleTime                  = 38,
	ProcessPagePriority               = 39,
	ProcessInstrumentationCallback    = 40,
	ProcessThreadStackAllocation      = 41,
	ProcessWorkingSetWatchEx          = 42,
	ProcessImageFileNameWin32         = 43,
	ProcessImageFileMapping           = 44,
	ProcessAffinityUpdateMode         = 45,
	ProcessMemoryAllocationMode       = 46,
	ProcessGroupInformation           = 47,
	ProcessTokenVirtualizationEnabled = 48,
	ProcessConsoleHostProcess         = 49,
	ProcessWindowInformation          = 50,
	ProcessHandleInformation          = 51,
	MaxProcessInfoClass               = 52
};

enum class JOBOBJECTINFOCLASS : unsigned int {
	JobObjectBasicAccountingInformation         = 1,
	JobObjectBasicLimitInformation              = 2,
	JobObjectBasicProcessIdList                 = 3,
	JobObjectBasicUIRestrictions                = 4,
	JobObjectSecurityLimitInformation           = 5, // deprecated
	JobObjectEndOfJobTimeInformation            = 6,
	JobObjectAssociateCompletionPortInformation = 7,
	JobObjectBasicAndIoAccountingInformation    = 8,
	JobObjectExtendedLimitInformation           = 9,
	JobObjectJobSetInformation                  = 10,
	JobObjectGroupInformation                   = 11,
	JobObjectNotificationLimitInformation       = 12,
	JobObjectLimitViolationInformation          = 13,
	JobObjectGroupInformationEx                 = 14,
	JobObjectCpuRateControlInformation          = 15,
	JobObjectCompletionFilter                   = 16,
	JobObjectCompletionCounter                  = 17,

	//
	//

	JobObjectFreezeInformation              = 18,
	JobObjectExtendedAccountingInformation  = 19,
	JobObjectWakeInformation                = 20,
	JobObjectBackgroundInformation          = 21,
	JobObjectSchedulingRankBiasInformation  = 22,
	JobObjectTimerVirtualizationInformation = 23,
	JobObjectCycleTimeNotification          = 24,
	JobObjectClearEvent                     = 25,
	JobObjectInterferenceInformation        = 26,
	JobObjectClearPeakJobMemoryUsed         = 27,
	JobObjectMemoryUsageInformation         = 28,
	JobObjectSharedCommit                   = 29,
	JobObjectContainerId                    = 30,
	JobObjectIoRateControlInformation       = 31,
	JobObjectNetRateControlInformation      = 32,
	JobObjectNotificationLimitInformation2  = 33,
	JobObjectLimitViolationInformation2     = 34,
	JobObjectCreateSilo                     = 35,
	JobObjectSiloBasicInformation           = 36,
	JobObjectReserved15Information          = 37,
	JobObjectReserved16Information          = 38,
	JobObjectReserved17Information          = 39,
	JobObjectReserved18Information          = 40,
	JobObjectReserved19Information          = 41,
	JobObjectReserved20Information          = 42,
	JobObjectReserved21Information          = 43,
	JobObjectReserved22Information          = 44,
	JobObjectReserved23Information          = 45,
	JobObjectReserved24Information          = 46,
	JobObjectReserved25Information          = 47,
	JobObjectReserved26Information          = 48,
	JobObjectReserved27Information          = 49,
	MaxJobObjectInfoClass                   = 50
};
#pragma warning( push )
#pragma warning( disable : 4201 )

struct _JOBOBJECT_WAKE_FILTER {
	unsigned long HighEdgeFilter;
	unsigned long LowEdgeFilter;
};
using JOBOBJECT_WAKE_FILTER  = _JOBOBJECT_WAKE_FILTER;
using PJOBOBJECT_WAKE_FILTER = _JOBOBJECT_WAKE_FILTER*;

// private
struct _JOBOBJECT_FREEZE_INFORMATION {
	union {
		unsigned long Flags;
		struct {
			unsigned long FreezeOperation : 1;
			unsigned long FilterOperation : 1;
			unsigned long SwapOperation : 1;
			unsigned long Reserved : 29;
		};
	};
	bool Freeze;
	bool Swap;
	bool Reserved0[ 2 ];
	JOBOBJECT_WAKE_FILTER WakeFilter;
};
#pragma warning( pop )

using JOBOBJECT_FREEZE_INFORMATION  = _JOBOBJECT_FREEZE_INFORMATION;
using PJOBOBJECT_FREEZE_INFORMATION = _JOBOBJECT_FREEZE_INFORMATION*;

} // namespace task_manager::detail::nt
