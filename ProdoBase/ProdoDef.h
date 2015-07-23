#pragma once

#include <ntifs.h>
#include <ntstrsafe.h>
#include <ntintsafe.h>

#define OBJECT_TO_OBJECT_HEADER( o ) \
	CONTAINING_RECORD( (o), OBJECT_HEADER, Body )

#define SystemProcessesAndThreadsInformation 5
#define SystemHandleInformation 16

#define ARKIT_NT_PROCESS_LIMIT          0x4e1c //0x41dc
#define PROCESS_STATE_EXITING           0x04 //0x000000fb
#define PROCESS_STATE_DELETED           0x08 //0x000000f7
#define THREAD_TERMINATED               1
#define THREAD_DEAD                     2

#define PS_CROSS_THREAD_FLAGS_SYSTEM 0x00000010UL 

#define NT_PROCESS_LIMIT 0x4e1c 

#pragma pack(1)
typedef struct ServiceDescriptorEntry {
	unsigned int *ServiceTableBase;
	unsigned int *ServiceCounterTableBase;
	unsigned int NumberOfServices;
	unsigned char *ParamTableBase;
} ServiceDescriptorTableEntry_t, *PServiceDescriptorTableEntry_t;
#pragma pack()

typedef unsigned long *PDWORD;
typedef unsigned char *PBYTE;

__declspec(dllimport) ServiceDescriptorTableEntry_t KeServiceDescriptorTable;

NTKERNELAPI HANDLE PsGetProcessId( __in PEPROCESS Process );
NTKERNELAPI HANDLE PsGetThreadId(__in PETHREAD Thread);
NTKERNELAPI HANDLE PsGetThreadProcessId( __in PETHREAD Thread );

NTKERNELAPI
	UCHAR *
	PsGetProcessImageFileName(
	__in PEPROCESS Process
	);

NTSYSAPI
	NTSTATUS
	NTAPI
	ZwQuerySystemInformation(    
	DWORD    SystemInformationClass,
	PVOID    SystemInformation,
	ULONG    SystemInformationLength,
	PULONG    ReturnLength
	);

NTSYSAPI
	NTSTATUS
	NTAPI
	ZwQueryInformationProcess(
	IN HANDLE ProcessHandle,
	IN PROCESSINFOCLASS ProcessInformationClass,
	OUT PVOID ProcessInformation,
	IN ULONG ProcessInformationLength,
	OUT PULONG ReturnLength OPTIONAL
	);

/** APC KeInitializeApc KeInsertQueueApc **/
typedef enum _KAPC_ENVIRONMENT
{
	OriginalApcEnvironment,
	AttachedApcEnvironment,
	CurrentApcEnvironment,
	InsertApcEnvironment
} KAPC_ENVIRONMENT, *PKAPC_ENVIRONMENT;

NTKERNELAPI VOID KeInitializeApc(
	PKAPC Apc,
	PKTHREAD Thread,
	KAPC_ENVIRONMENT Environment,
	PKKERNEL_ROUTINE KernelRoutine,
	PKRUNDOWN_ROUTINE RundownRoutine,
	PKNORMAL_ROUTINE NormalRoutine,
	KPROCESSOR_MODE ProcessorMode,
	PVOID NormalContext
	);

NTKERNELAPI BOOLEAN KeInsertQueueApc(
	PRKAPC Apc,
	PVOID SystemArgument1,
	PVOID SystemArgument2,
	KPRIORITY Increment
	);

/** XP ~ WIN7 ExEnumHandleTable function and struct **/

typedef struct _HANDLE_TRACE_DB_ENTRY
{
	CLIENT_ID ClientId;
	PVOID Handle;
	ULONG Type;
	VOID * StackTrace[16];
} HANDLE_TRACE_DB_ENTRY, *PHANDLE_TRACE_DB_ENTRY;

typedef struct _HANDLE_TRACE_DEBUG_INFO
{
	LONG RefCount;
	ULONG TableSize;
	ULONG BitMaskFlags;
	FAST_MUTEX CloseCompactionLock;
	ULONG CurrentStackIndex;
	HANDLE_TRACE_DB_ENTRY TraceDb[1];
} HANDLE_TRACE_DEBUG_INFO, *PHANDLE_TRACE_DEBUG_INFO;

typedef struct _HANDLE_TABLE_ENTRY_INFO
{
	ULONG AuditMask;
} HANDLE_TABLE_ENTRY_INFO, *PHANDLE_TABLE_ENTRY_INFO;

typedef struct _HANDLE_TABLE_ENTRY
{
	union
	{
		PVOID Object;
		ULONG ObAttributes;
		PHANDLE_TABLE_ENTRY_INFO InfoTable;
		ULONG Value;
	};
	union
	{
		ULONG GrantedAccess;
		struct
		{
			WORD GrantedAccessIndex;
			WORD CreatorBackTraceIndex;
		};
		LONG NextFreeTableEntry;
	};
} HANDLE_TABLE_ENTRY, *PHANDLE_TABLE_ENTRY;

typedef struct _HANDLE_TABLE
{
	ULONG TableCode;
	PEPROCESS QuotaProcess;
	PVOID UniqueProcessId;
	EX_PUSH_LOCK HandleLock;
	LIST_ENTRY HandleTableList;
	EX_PUSH_LOCK HandleContentionEvent;
	PHANDLE_TRACE_DEBUG_INFO DebugInfo;
	LONG ExtraInfoPages;
	ULONG Flags;
	ULONG StrictFIFO: 1;
	LONG FirstFreeHandle;
	PHANDLE_TABLE_ENTRY LastFreeHandleEntry;
	LONG HandleCount;
	ULONG NextHandleNeedingPool;
} HANDLE_TABLE, *PHANDLE_TABLE;

typedef struct _SYSTEM_HANDLE_TABLE_ENTRY_INFO
{
	USHORT UniqueProcessId;
	USHORT CreatorBackTraceIndex;
	UCHAR ObjectTypeIndex;
	UCHAR HandleAttributes;
	USHORT HandleValue;
	PVOID Object;
	ULONG GrantedAccess;
} SYSTEM_HANDLE_TABLE_ENTRY_INFO, *PSYSTEM_HANDLE_TABLE_ENTRY_INFO;

typedef BOOLEAN (NTAPI *PEX_ENUM_HANDLE_CALLBACK)(
	__inout PHANDLE_TABLE_ENTRY HandleTableEntry,
	__in HANDLE Handle,
	__in PVOID Context
	);

NTKERNELAPI
	BOOLEAN
	NTAPI
	ExEnumHandleTable(
	IN PHANDLE_TABLE HandleTable,
	IN PEX_ENUM_HANDLE_CALLBACK EnumHandleProcedure,
	IN OUT PVOID Context,
	OUT PHANDLE Handle OPTIONAL
	);

typedef struct _OBJECT_CREATE_INFORMATION {
	ULONG Attributes;
	HANDLE RootDirectory;
	PVOID ParseContext;
	KPROCESSOR_MODE ProbeMode;
	ULONG PagedPoolCharge;
	ULONG NonPagedPoolCharge;
	ULONG SecurityDescriptorCharge;
	PSECURITY_DESCRIPTOR SecurityDescriptor;
	PSECURITY_QUALITY_OF_SERVICE SecurityQos;
	SECURITY_QUALITY_OF_SERVICE SecurityQualityOfService;
} OBJECT_CREATE_INFORMATION, *POBJECT_CREATE_INFORMATION;

typedef struct _OBJECT_HEADER {
	LONG_PTR PointerCount;
	union {
		LONG_PTR HandleCount;
		PVOID NextToFree;
	};
	POBJECT_TYPE Type;
	UCHAR NameInfoOffset;
	UCHAR HandleInfoOffset;
	UCHAR QuotaInfoOffset;
	UCHAR Flags;

	union {
		POBJECT_CREATE_INFORMATION ObjectCreateInfo;
		PVOID QuotaBlockCharged;
	};

	PSECURITY_DESCRIPTOR SecurityDescriptor;
	QUAD Body;
} OBJECT_HEADER, *POBJECT_HEADER;

typedef struct _LDR_MODULE {
	LIST_ENTRY InLoadOrderModuleList;
	LIST_ENTRY InMemoryOrderModuleList;
	LIST_ENTRY InInitializationOrderModuleList;
	PVOID BaseAddress;
	PVOID EntryPoint;
	ULONG SizeOfImage;
	UNICODE_STRING FullDllName;
	UNICODE_STRING BaseDllName;
	ULONG Flags;
	SHORT LoadCount;
	SHORT TlsIndex;
	LIST_ENTRY HashTableEntry;
	ULONG TimeDateStamp;
} LDR_MODULE, *PLDR_MODULE;

typedef struct _SYSTEM_PROCESS_INFORMATION {
	ULONG NextEntryOffset;
	BYTE Reserved1[52];
	PVOID Reserved2[3];
	HANDLE UniqueProcessId;
	PVOID Reserved3;
	ULONG HandleCount;
	BYTE Reserved4[4];
	PVOID Reserved5[11];
	SIZE_T PeakPagefileUsage;
	SIZE_T PrivatePageCount;
	LARGE_INTEGER Reserved6[6];
} SYSTEM_PROCESS_INFORMATION, *PSYSTEM_PROCESS_INFORMATION;

typedef struct _SYSTEM_HANDLE_INFORMATION
{
	ULONG NumberOfHandles;
	SYSTEM_HANDLE_TABLE_ENTRY_INFO Handles[1];
} SYSTEM_HANDLE_INFORMATION, *PSYSTEM_HANDLE_INFORMATION;

typedef struct _SE_AUDIT_PROCESS_CREATION_INFO
{
	POBJECT_NAME_INFORMATION ImageFileName;
} SE_AUDIT_PROCESS_CREATION_INFO, *PSE_AUDIT_PROCESS_CREATION_INFO;

typedef struct _CONTROL_AREA {
	//CONTROL_AREA Strutct for winxp
	PVOID Segment; //PSEGMENT
	LIST_ENTRY DereferenceList;
	ULONG NumberOfSectionReferences;    // All section refs & image flushes
	ULONG NumberOfPfnReferences;        // valid + transition prototype PTEs
	ULONG NumberOfMappedViews;          // total # mapped views, including
	// system cache & system space views
	USHORT NumberOfSubsections;         // system cache views only
	USHORT FlushInProgressCount;
	ULONG NumberOfUserReferences;       // user section & view references
	ULONG LongFlags;
	PFILE_OBJECT FilePointer;
	PVOID WaitingForDeletion;           //PEVENT_COUNTER
	USHORT ModifiedWriteCount;
	USHORT NumberOfSystemCacheViews;
} CONTROL_AREA, *PCONTROL_AREA;

typedef struct _MMVAD {
	//MMVAD Struct for winxp
	ULONG_PTR StartingVpn;
	ULONG_PTR EndingVpn;
	struct _MMVAD *Parent;
	struct _MMVAD *LeftChild;
	struct _MMVAD *RightChild;
	ULONG_PTR LongFlags;
	PCONTROL_AREA ControlArea;
	PVOID FirstPrototypePte; //PMMPTE
	PVOID LastContiguousPte; //PMMPTE
	ULONG LongFlags2;
} MMVAD, *PMMVAD;
