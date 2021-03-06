﻿// strongOD_dll.cpp : 定义 DLL 应用程序的导出函数。


#include <Windows.h>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include "plugin.h"
#include "Ioctl.h"
#include "func.h"

#pragma comment(lib, "ollydbg.lib")


char g_szPluginName[] = "aadbg";
HWND g_hWndMain = NULL;
HINSTANCE g_hModule = NULL;




BOOL APIENTRY DllMain(
	HINSTANCE hModule,
	DWORD reason,
	LPVOID lpReserved)
{
	if (DLL_PROCESS_ATTACH == reason)
	{
		g_hModule = hModule;
	}

	return TRUE;
}


extc int _export cdecl ODBG_Plugindata(
	char shortname[32])
{
	strcpy(shortname, g_szPluginName);

	return PLUGIN_VERSION;
}


HANDLE g_hSys = NULL;

extc int _export cdecl ODBG_Plugininit(
	int ollydbgversion,
	HWND hw,
	ulong* features)
{

	if (ollydbgversion < PLUGIN_VERSION)
		return -1;

	g_hWndMain = hw; // od的窗口句柄

	if (!SetPrivilege(SE_DEBUG_NAME, TRUE))
	{
		MessageBox(g_hWndMain, "SetPrivilege failed!", g_szPluginName, MB_OK);
		return -1;
	}

	//int hook_cnt = HookAllProcess();
	//char str_hookcnt[64] = {0};
	//sprintf(str_hookcnt, "hook cnt: %d", hook_cnt);
	//MessageBox(g_hWndMain, str_hookcnt, g_szPluginName, MB_OK);

	// 调用aadbg_sys.sys驱动程序，hook ssdt和shadowssdt等
	//g_hSys = CreateFile("\\.\\aadbg_sys", FILE_ALL_ACCESS, FILE_SHARE_READ, NULL, 0, 0, NULL);
	//if (g_hSys == INVALID_HANDLE_VALUE || g_hSys == NULL)
	//	return -1;
	//BOOL bRet = DeviceIoControl(g_hSys, IOCTL_HOOK_SSDT, NULL, 0, NULL, 0, NULL, NULL);
	//if (!bRet)
	//	return -1;


	// 加载信息输出到log window，od启动后alt+l即可看到
	//MessageBox(g_hWndMain, "plugin init", g_szPluginName, MB_OK);
	Addtolist(0, 0, "aadbg v1.0");
	Addtolist(0, -1, "    Copyright (C) 2018 Xiang");

	return 0;

}

int ModifyPEBFlag(DWORD pid, DWORD tid);

extc int _export cdecl ODBG_Pausedex(int reason, int dummy, t_reg* reg, DEBUG_EVENT* debug_event)
{
	//char sreason[64] = {0};
	//sprintf(sreason, "pausedex reason: %d", reason);
	//MessageBox(g_hWndMain, sreason, g_szPluginName, MB_OK);

	//char sinfo[64] = {0};
	//sprintf(sinfo, "pid: %08X, tid: %08X", debug_event->dwProcessId, debug_event->dwThreadId);
	//MessageBox(g_hWndMain, sinfo, g_szPluginName, MB_OK);

	if (reason != 0x10)
	{
		return 1;
	}

	static int BE_HOOKED = 0;
	if (0 == BE_HOOKED)
	{
		int ret = ModifyPEBFlag(debug_event->dwProcessId, debug_event->dwThreadId);
		if (ret == 0)
		{
			MessageBox(g_hWndMain, "Modify PEB failed", g_szPluginName, MB_OK);
		}

		

		BE_HOOKED = 1;
	}

	return 1;
}


#if 0

extc void _export cdecl ODBG_Pluginmainloop(DEBUG_EVENT* debug_event)
{
	if (debug_event == NULL) return;
	
	DWORD dwPID = debug_event->dwProcessId;

	if (dwPID == GetCurrentProcessId())
	{
		return;
	}

	if( CREATE_PROCESS_DEBUG_EVENT == debug_event->dwDebugEventCode )
	{

		HANDLE hProcess = debug_event->u.CreateProcessInfo.hProcess;
#if 0
		// 使用hook的方法
		if (!HookProcess(dwPID, TRUE, hProcess))
		{
			MessageBox(g_hWndMain, "hook failed", g_szPluginName, MB_OK);
		}

#else

		int ret = ModifyPEBFlag(hProcess);


#endif

	} // if event create process
	// 根本没有这个事件。。。。。。。。。。。。。。。。
	else if (CREATE_THREAD_DEBUG_EVENT == debug_event->dwDebugEventCode)
	{
		//MessageBox(g_hWndMain, "CREATE_THREAD_DEBUG_EVENT", g_szPluginName, MB_OK);

	} // else if create thread event
}

#endif

int ModifyPEBFlag(DWORD pid, DWORD tid)
{
#if 1
	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
	HMODULE NtdllModule = GetModuleHandle(_T("ntdll.dll"));
	PFN_NTQIP NtQueryInformationProcess = (PFN_NTQIP)GetProcAddress(NtdllModule,
		"NtQueryInformationProcess");
	PROCESS_BASIC_INFORMATION32 pbi = { 0 };
	UINT32  ReturnLength = 0;
	NTSTATUS Status = NtQueryInformationProcess(hProcess,
		ProcessBasicInformation, &pbi, (UINT32)sizeof(pbi), (UINT32*)&ReturnLength);

	char status_str[32] = { 0 };
	sprintf(status_str, "ntstatus: %d", Status);
	MessageBox(g_hWndMain, status_str, g_szPluginName, MB_OK);

	if (NT_SUCCESS(Status))
	{

		//_PEB32* pPEB = (_PEB32*)malloc(sizeof(_PEB32));
		//Status = ReadProcessMemory(hProcess, (PVOID)pbi.PebBaseAddress, (_PEB32*)pPEB, sizeof(_PEB32), NULL);
		//_RTL_USER_PROCESS_PARAMETERS32 Parameters32;
		//Status = ReadProcessMemory(hProcess, (PVOID)pPEB->ProcessParameters, &Parameters32, sizeof(_RTL_USER_PROCESS_PARAMETERS32), NULL);
		//BYTE* Environment = new BYTE[Parameters32.EnvironmentSize * 2];
		//Status = ReadProcessMemory(hProcess, (PVOID)Parameters32.Environment, Environment, Parameters32.EnvironmentSize, NULL);

		DWORD old_protect;
		if (!VirtualProtectEx(hProcess, (LPVOID)pbi.PebBaseAddress, 4096, PAGE_READWRITE, &old_protect))
		{
			MessageBox(g_hWndMain, "virtualprotectex change failed", g_szPluginName, MB_OK);
			return 0;
		}

		byte pPEB[512] = { 0 };
		DWORD NumOfBytes;
		if (!ReadProcessMemory(hProcess, (PVOID)pbi.PebBaseAddress, pPEB, 512, &NumOfBytes)
			|| NumOfBytes == 0)
		{
			MessageBox(g_hWndMain, "read peb failed", g_szPluginName, MB_OK);
			return 0;
		}

		char tmp[1024] = { 0 };
		char* ptmp = tmp;
		for (int i = 0; i < 512; ++i, ++ptmp)
		{
			sprintf(ptmp, "%02X ", pPEB[i]);
		}
		MessageBox(g_hWndMain, tmp, g_szPluginName, MB_OK);

		// 修改DebugFlag
		pPEB[2] = 0;
		// 修改NtGlobalFlag
		DWORD dwNtGlobalFlag = *(LPDWORD)(pPEB + 0x68);
		dwNtGlobalFlag &= ~0x70;
		*(LPDWORD)(pPEB + 0x68) = dwNtGlobalFlag;

		if (!WriteProcessMemory(hProcess, (PVOID)pbi.PebBaseAddress, pPEB, 512, &NumOfBytes)
			|| NumOfBytes == 0)
		{
			MessageBox(g_hWndMain, "write peb failed", g_szPluginName, MB_OK);
			return 0;
		}

		if (!VirtualProtectEx(hProcess, (LPVOID)pbi.PebBaseAddress, 4096, old_protect, &old_protect))
		{
			MessageBox(g_hWndMain, "virtualprotectex restore failed", g_szPluginName, MB_OK);
			return 0;
		}

		return 1;

	} // if ntqip success

#else

	DWORD tib, pib;
	LDT_ENTRY segselector;
	CONTEXT TempContext;

	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
	HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, tid);
	if (NULL == hProcess || NULL == hThread)
		return 0;

	// 把段相关的地址转换为线性地址
	// ReadProcessMemory和WriteProcessMemory用的是线性地址
	TempContext.ContextFlags = CONTEXT_SEGMENTS;
	GetThreadContext(hThread, &TempContext);
	GetThreadSelectorEntry(hThread, TempContext.SegFs, &segselector);
	tib = ((segselector.HighWord.Bytes.BaseHi) << 24) +
		((segselector.HighWord.Bytes.BaseMid) << 16) +
		(segselector.BaseLow);

	if (ReadProcessMemory(hProcess, (void *)(tib + 0x30), &pib, sizeof(pib), NULL) == 0)
	{
		return 0;
	}

	char debug_info = 0xFF;
	pib += 2;
	if (ReadProcessMemory(hProcess, (void *)pib, &debug_info, sizeof(debug_info), NULL) == 0)
	{
		return 0;
	}

	if (debug_info != 0x01)
	{
		return 0;
	}

	debug_info = 0;
	if (WriteProcessMemory(hProcess, (void *)pib, &debug_info, sizeof(debug_info), NULL) == 0)
	{
		return 0;
	}

	return 1;

#endif

	return 0;
}


extc int _export cdecl ODBG_Pluginmenu(
	int origin,
	char data[4096],
	void* item)
{
	//if (PM_MAIN == origin)
	//{
	//	// breakall可以，break all就不行，有空格会影响
	//	strcpy(data, "0 Hello | 1 breakall | 2 About");
	//	return 1;
	//}

	t_dump* pd;

	switch (origin)
	{
	case PM_MAIN: // 表示菜单项
		strcpy(data, "1 breakall | 2 About");
		return 1;
	case PM_DISASM: // 表示界面中右键菜单项
		pd = (t_dump*)item;
		if (pd == NULL)
			return 0;
		strcpy(data, "1 breakall");
		return 1;
	default:
		break;
	}

	return 0;
}


// 菜单响应
extc void _export cdecl ODBG_Pluginaction(
	int origin,
	int action,
	void* item)
{
	//if (PM_MAIN == origin)
	//{
		switch (action)
		{
		case 1:
			//MessageBox(g_hWndMain, "break all.", g_szPluginName, MB_OK);


			break;
		case 2:
			MessageBox(g_hWndMain, "Writen by Xiang", g_szPluginName, MB_OK);
			break;
		}
	//}
}



// 重新打开程序
extc void _export cdecl ODBG_Pluginreset(void)
{
	Addtolist(0, -1, "reset");
}


// WM_CLOSE消息尚未发送
extc int _export cdecl ODBG_Pluginclose(void)
{
	//MessageBox(g_hWndMain, "plugin close", g_szPluginName, MB_OK);
	return 0;
}

// WM_DESTROY消息已经收到
extc void _export cdecl ODBG_Plugindestroy(void)
{
	//UnhookAllProcess();

	//BOOL bRet = DeviceIoControl(g_hSys, IOCTL_UNHOOK_SSDT, NULL, 0, NULL, 0, NULL, NULL);

	//MessageBox(NULL, "plugin destroy", g_szPluginName, MB_OK);
	
}