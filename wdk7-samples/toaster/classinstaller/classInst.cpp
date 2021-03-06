//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  File:       ClassINST.C
//
//  Contents:   class-installer for toaster.
//
//  Notes:      This is a sample class installer. Class installer is not required 
//              for every class of device. I have included here to provide 
//              an icon for the toaster class and to demonstrate how one
//              can provide a customer property sheet in the device manager
//              in response to DIF_ADDPROPERTYPAGE_ADVANCED. 

//              The property sheet has a check box. If you enable the box
//              and press OK, it will restart the device. This is required
//              if the user changes the device properties, and it needs to
//              be restarted to apply the new attributes. // 这段注释明显已经过时了,却没有删除

//              For a complete description of ClassInstallers, please see the 
//              Microsoft Windows 2000 DDK Documentation.
//  
//  Revision: Added property page - Nov 14, 2000
//  

#ifndef DONT_USE_WDK // Chj: when compiled with VS2010 vcxproj, don't use WDK headers
// Annotation to indicate to prefast that this is non-driver user-mode code.
#include <DriverSpecs.h>
__user_code  
#endif

#include <windows.h>
#include <setupapi.h>
#include <Cfgmgr32.h> // 
#include <assert.h>
#include <stdio.h>
#include <tchar.h>
#include "resource.h"

#ifndef DONT_USE_WDK 
//#include <dontuse.h> // E:\WinDDK\7600.16385.1\inc\api\dontuse.h
#endif

//+---------------------------------------------------------------------------
//
// WARNING! 
//
// Installer(such as this DLL) must not show any popup to the user.
// It should provide appropriate defaults.
//
//  OutputDebugString should be fine...
//
#if (defined DBG) || (defined _DEBUG)
#define DbgOut(Text) OutputDebugString(TEXT("ClassInstaller: " Text "\n"))
#else
#define DbgOut(Text) 
#endif 

typedef struct _TOASTER_PROP_PARAMS
{
	HDEVINFO                     DeviceInfoSet;
	PSP_DEVINFO_DATA             DeviceInfoData;
	BOOL                         Restart;  
} TOASTER_PROP_PARAMS, *PTOASTER_PROP_PARAMS;


INT_PTR CALLBACK
PropPageDlgProc(__in HWND   hDlg,
               __in UINT   uMessage,
               __in WPARAM wParam,
               __in LPARAM lParam);

UINT CALLBACK
PropPageDlgCallback(HWND hwnd,
                    UINT uMsg,
                    LPPROPSHEETPAGE ppsp);
DWORD
PropPageProvider(__in HDEVINFO DeviceInfoSet, 
	__in PSP_DEVINFO_DATA DeviceInfoData OPTIONAL); // Chj: this should be marked as OPTIONAL?

BOOL
OnNotify(HWND ParentHwnd, LPNMHDR NmHdr, PTOASTER_PROP_PARAMS Params);
    
HMODULE g_ModuleInstance;

BOOL WINAPI 
DllMain(HINSTANCE DllInstance, DWORD Reason, PVOID Reserved)
{
    UNREFERENCED_PARAMETER( Reserved );

    switch(Reason) 
	{{
        case DLL_PROCESS_ATTACH: {

            g_ModuleInstance = DllInstance;
            DisableThreadLibraryCalls(DllInstance);
            InitCommonControls();
            break;
        }

        case DLL_PROCESS_DETACH: {
            g_ModuleInstance = NULL;
            break;
        }

        default: {
            break;
        }
	}}

    return TRUE;
}

void check_DIF_PROPERTYCHANGE(HDEVINFO dis, PSP_DEVINFO_DATA did) // chj test
{
	SP_PROPCHANGE_PARAMS cinfo = {{sizeof(SP_CLASSINSTALL_HEADER)}}; // (setup-)class-info
	SP_DEVINSTALL_PARAMS dinfo = {sizeof(SP_DEVINSTALL_PARAMS)}; // device info
	BOOL b = 0;
//	const int dbgsize = 100; // Not valid for C (ok for c++)
#define dbgsize 100
	TCHAR buf[dbgsize];

	b = SetupDiGetClassInstallParams(dis, did,
		(PSP_CLASSINSTALL_HEADER)&cinfo, //[out]
		sizeof(SP_PROPCHANGE_PARAMS), NULL);
	assert(b);
	assert(DIF_PROPERTYCHANGE==cinfo.ClassInstallHeader.InstallFunction);

	_sntprintf_s(buf, dbgsize, _TRUNCATE, 
		TEXT("> ClassInstallParams: .StateChange=%d, .Scope=%d, .HwProfile=%d\n"),
		cinfo.StateChange, cinfo.Scope, cinfo.HwProfile);
	OutputDebugString(buf);

	b = SetupDiGetDeviceInstallParams(dis, did, &dinfo);
	assert(b);

	_sntprintf_s(buf, dbgsize, _TRUNCATE, 
		TEXT("> DeviceInstallParams: .Flags=0x%X, .FlagsEx=0x%X\n"),
		dinfo.Flags, dinfo.FlagsEx);
	OutputDebugString(buf);
}

DWORD CALLBACK
ToasterClassInstaller( // The default name for the function is ClassInstall
    __in  DI_FUNCTION         InstallFunction,
    __in  HDEVINFO            DeviceInfoSet,
    __in  PSP_DEVINFO_DATA    DeviceInfoData OPTIONAL
    )
/*++
Routine Description: 
    Responds to Class-installer messages
    .  
Arguments:
     InstallFunction   [in] 
     DeviceInfoSet     [in]
     DeviceInfoData    [in]

Returns: ERROR_DI_DO_DEFAULT, NO_ERROR, or an error code. (Chj fixed M$'s wrong comment)
Note: return code ERROR_DI_POSTPROCESSING_REQUIRED is used for CoInstaller, not ClassInstaller.
--*/
{
    switch (InstallFunction)
    {
        case DIF_INSTALLDEVICE: 
            //
            // Sent twice: first before installing the device and 
            // [ second after installing device, if you have returned 
            // ERROR_DI_POSTPROCESSING_REQUIRED during the first pass ].
            //
            DbgOut("DIF_INSTALLDEVICE");
            break;
            
        case DIF_ADDPROPERTYPAGE_ADVANCED:
            //
            // Sent when you check the properties of the device in the
            // device manager.
            //
            DbgOut("DIF_ADDPROPERTYPAGE_ADVANCED");
            return PropPageProvider(DeviceInfoSet, DeviceInfoData);           
            
        case DIF_POWERMESSAGEWAKE:
            //
            // Sent when you check the power management tab 
            //
            DbgOut("DIF_POWERMESSAGEWAKE");
            break;

        case DIF_PROPERTYCHANGE:
            //
            // Sent when you change the property of the device using
            // SetupDiSetDeviceInstallParams. (Enable/Disable/Restart)
            //
            DbgOut("DIF_PROPERTYCHANGE");
			check_DIF_PROPERTYCHANGE(DeviceInfoSet, DeviceInfoData); // chj test
            break;
        case DIF_REMOVE: 
             //
             // Sent when you uninstall the device.
             //
             DbgOut("DIF_REMOVE");
             break;
             
        case DIF_NEWDEVICEWIZARD_FINISHINSTALL:
            //
            // Sent near the end of installation to allow 
            // an installer to supply wizard page(s) to the user.
            // These wizard pages are different from the device manager
            // property sheet.There are popped only once during install.
            //
            DbgOut("DIF_NEWDEVICEWIZARD_FINISHINSTALL");
            break;
            
        case DIF_SELECTDEVICE:
            DbgOut("DIF_SELECTDEVICE");
            break;
        case DIF_DESTROYPRIVATEDATA:
            //
            // Sent when Setup destroys a device information set 
            // or an SP_DEVINFO_DATA element, or when Setup discards 
            // its list of co-installers and class installer for a device
            //
            DbgOut("DIF_DESTROYPRIVATEDATA");
            break;
        case DIF_INSTALLDEVICEFILES:
            DbgOut("DIF_INSTALLDEVICEFILES");
            break;
        case DIF_ALLOW_INSTALL:
            //
            // Sent to confirm whether the installer wants to allow
            // the installation of device.
            //
            DbgOut("DIF_ALLOW_INSTALL");
            break;
        case DIF_SELECTBESTCOMPATDRV:
            DbgOut("DIF_SELECTBESTCOMPATDRV");
            break;

        case DIF_INSTALLINTERFACES:
            DbgOut("DIF_INSTALLINTERFACES");
            break;
        case DIF_REGISTER_COINSTALLERS:
            DbgOut("DIF_REGISTER_COINSTALLERS");
            break;
        default:
            DbgOut("DIF_???");
            break;
    }   
    return ERROR_DI_DO_DEFAULT;    
}

DWORD
PropPageProvider(
    __in  HDEVINFO            DeviceInfoSet,
    __in  PSP_DEVINFO_DATA    DeviceInfoData OPTIONAL
)
/*++
Routine Description: 
    Entry-point for adding additional device manager property-sheet pages.  

Returns:    NO_ERROR, ERROR_DI_DO_DEFAULT, or an error code.

本函数应该遵循的处理过程在 WDK7 chm 标题 "Handling DIF_ADDPROPERTYPAGE_ADVANCED Requests"
或在线网址(2016-08-28) https://msdn.microsoft.com/windows/hardware/drivers/install/creating-custom-property-pages

执行时机: 刚刚打开设备管理器的 ToasterDevice01 属性对话框时, 此函数就会被执行.
--*/
{
    HPROPSHEETPAGE  pageHandle;
    PROPSHEETPAGE   page;
    PTOASTER_PROP_PARAMS      params = NULL;
	SP_ADDPROPERTYPAGE_DATA AddPropertyPageData = {{sizeof(SP_CLASSINSTALL_HEADER)}};

    //
    // DeviceInfoSet is NULL if setup is requesting property pages for
    // the device setup class. We don't want to do anything in this 
    // case.
    //
    if (DeviceInfoData==NULL) {
        return ERROR_DI_DO_DEFAULT;
    }

    //
    // Get the current class install parameters for the device
    //

    if (SetupDiGetClassInstallParams(DeviceInfoSet, DeviceInfoData,
         (PSP_CLASSINSTALL_HEADER)&AddPropertyPageData, //[out]
         sizeof(SP_ADDPROPERTYPAGE_DATA), NULL )) 
    {
		assert(DIF_ADDPROPERTYPAGE_ADVANCED==AddPropertyPageData.ClassInstallHeader.InstallFunction);
			// Chj: Only when this assert is ok, the output data from 
			// SetupDiGetClassInstallParams is an SP_ADDPROPERTYPAGE_DATA struct.

        // Ensure that the maximum number of dynamic pages for the 
        // device has not yet been met
        //
        if(AddPropertyPageData.NumDynamicPages >= MAX_INSTALLWIZARD_DYNAPAGES){
            return NO_ERROR;
        }
        params = (TOASTER_PROP_PARAMS*)HeapAlloc(GetProcessHeap(), 0, sizeof(TOASTER_PROP_PARAMS));
			// Chj: this memblock is freed in PropPageDlgCallback().
        if (params)
        {
            //
            // Save DeviceInfoSet and DeviceInfoData
            //
            params->DeviceInfoSet     = DeviceInfoSet;
            params->DeviceInfoData    = DeviceInfoData;
            params->Restart           = FALSE;
            
            //
            // Create custom property sheet page
            //
            memset(&page, 0, sizeof(PROPSHEETPAGE));

            page.dwSize = sizeof(PROPSHEETPAGE); // 104 on x64
            page.dwFlags = PSP_USECALLBACK;
            page.hInstance = g_ModuleInstance; // e.g. 0x7FE`F4C50000 ; mmc.exe's base is 0xFF040000
            page.pszTemplate = MAKEINTRESOURCE(DLG_TOASTER_PORTSETTINGS);
            page.pfnDlgProc = PropPageDlgProc;
            page.pfnCallback = PropPageDlgCallback;

            page.lParam = (LPARAM) params;
				// Chj: this value will be retrieved in PropPageDlgProc.WM_INITDIALOG

            pageHandle = CreatePropertySheetPage(&page);
            if(!pageHandle)
            {
                HeapFree(GetProcessHeap(), 0, params);
                return NO_ERROR;
            }

            //
            // Add the new page to the list of dynamic property sheets
            //
            AddPropertyPageData.DynamicPages[
                AddPropertyPageData.NumDynamicPages++] = pageHandle;

            SetupDiSetClassInstallParams(DeviceInfoSet,
                        DeviceInfoData,
                        (PSP_CLASSINSTALL_HEADER)&AddPropertyPageData,
                        sizeof(SP_ADDPROPERTYPAGE_DATA));
        }
    }
    return NO_ERROR;
} 

INT_PTR CALLBACK
PropPageDlgProc(__in HWND   hDlg,
                   __in UINT   uMessage,
                   __in WPARAM wParam,
                   __in LPARAM lParam)
/*++
Routine Description: PropPageDlgProc
    The windows control function for the custom property page window

Arguments:
    hDlg, uMessage, wParam, lParam: standard windows DlgProc parameters

Return Value:
    BOOL: FALSE if function fails, TRUE if function passes
--*/
{
    PTOASTER_PROP_PARAMS params;
    UNREFERENCED_PARAMETER( wParam );

    params = (PTOASTER_PROP_PARAMS) GetWindowLongPtr(hDlg, DWLP_USER);

    switch(uMessage) 
	{{
    case WM_COMMAND:
        break;

    case WM_CONTEXTMENU:
        break;

    case WM_HELP:
        break;

    case WM_INITDIALOG: 
	{
		// 在用户点击 "Custom Property Page" 选项卡时才会撞到, 
		// 仅仅打开 ToasterDevice 属性对话框并不会撞到.

		BOOL fSuccess = FALSE;
		TCHAR origFriendlyName[100] = {0};

		DbgOut("PropPageDlgProc.WM_INITDIALOG");
		//
		// on WM_INITDIALOG call, lParam points to the property sheet page.
		//
		// The lParam field in the property sheet page struct is set by the
		// caller. This was set when we created the property sheet.
		// Save this in the user window long so that we can access it on later 
		// on later messages.
		//
		params = (PTOASTER_PROP_PARAMS) ((LPPROPSHEETPAGE)lParam)->lParam;
		SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR) params);

		// Chj add: Retrieve existing friendly-name and display it in the editbox.
		fSuccess = SetupDiGetDeviceRegistryProperty(params->DeviceInfoSet, params->DeviceInfoData,
			SPDRP_FRIENDLYNAME, NULL, (BYTE*)origFriendlyName, sizeof(origFriendlyName), NULL);
		if(fSuccess && origFriendlyName[0])
			SetDlgItemText(hDlg, IDC_FRIENDLYNAME, origFriendlyName);

		break;
	}
    case WM_NOTIFY:
        OnNotify(hDlg, (NMHDR *)lParam, params);
        break;

	case WM_DESTROY:
		// 关闭 ToasterDevice 属性对话框时撞到.
		DbgOut("PropPageDlgProc.WM_DESTROY"); // chj debug

    default: 
        return FALSE;
	}}

    return TRUE;
} 


UINT CALLBACK
PropPageDlgCallback(HWND hwnd,
                   UINT uMsg,
                   LPPROPSHEETPAGE ppsp)
{
    PTOASTER_PROP_PARAMS params;
    UNREFERENCED_PARAMETER( hwnd );

    switch (uMsg) 
	{{
    case PSPCB_CREATE:
        // 当第一次点击 "Custom Property Page" 选项卡时, 会撞到此处, 
		// 且在 PropPageDlgProc.WM_INITDIALOG 之前撞到.
		//
        // Called when the property sheet is first displayed
        //
        return TRUE;    // return TRUE to continue with creation of page

    case PSPCB_RELEASE:
		// 在 PropPageDlgProc.WM_DESTROY 之后撞到.
		// 但注意: 如果用户没有去点击"Custom Property Page" 选项卡, WM_INITDIALOG 和 WM_DESTROY 都不会被执行.
        //
        // Called when property page is destroyed, even if the page 
        // was never displayed. This is the correct way to release data.
        //
        params = (PTOASTER_PROP_PARAMS) ppsp->lParam;
        LocalFree(params); // was allocated in PropPageProvider()

        return 0;       // return value ignored
    default:
        break;
	}}

    return TRUE;
}


BOOL
OnNotify(
    HWND    ParentHwnd,
    LPNMHDR NmHdr,
    PTOASTER_PROP_PARAMS Params
    )
{
    SP_DEVINSTALL_PARAMS spDevInstall = {0};
    TCHAR                friendlyName[LINE_LEN] ={0};
    BOOL    fSuccess;
    
    switch (NmHdr->code) 
	{{
    case PSN_APPLY:
        //
        // Sent when the user clicks on Apply OR OK !!
        //
        GetDlgItemText(ParentHwnd, IDC_FRIENDLYNAME, friendlyName,
                                        LINE_LEN-1 );
        friendlyName[LINE_LEN-1] = UNICODE_NULL;
        if(friendlyName[0]) 
		{
            fSuccess = SetupDiSetDeviceRegistryProperty(Params->DeviceInfoSet, 
                         Params->DeviceInfoData,
                         SPDRP_FRIENDLYNAME,
                         (BYTE *)friendlyName,
                         (lstrlen(friendlyName)+1) * sizeof(TCHAR)
                         );
            if(!fSuccess) {
                DbgOut("SetupDiSetDeviceRegistryProperty failed!");                   
                break;
            }

			// Chj Q: 一定要让 ToasterDevice 经历 IRP_MN_STOP_DEVICE/IRP_MN_START_DEVICE 吗?
			// 有没办法仅仅让设备管理器执行一次"扫描硬件改动"(那样也可以刷新 friendly name 显示的)? 

#define WILL_RESTART_DEVICE // WDK toaster default code
#ifdef  WILL_RESTART_DEVICE 
			//
			// Inform setup about property change so that it can restart the device.
			//
			
			spDevInstall.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
     
            if (Params && SetupDiGetDeviceInstallParams(Params->DeviceInfoSet,
                                              Params->DeviceInfoData,
                                              &spDevInstall)) 
			{
				// Chj: 发现 spDevInstall 结构体中返回的所有成员都是零值，这说明什么？

                // If your device requires a reboot to restart, you can
                // specify that by setting DI_NEEDREBOOT as shown below
                //
                // if(NeedReboot) {
                //    spDevInstall.Flags |= DI_PROPERTIES_CHANGE | DI_NEEDREBOOT;
                // }
                
                spDevInstall.FlagsEx |= DI_FLAGSEX_PROPCHANGE_PENDING;
                	// Chj: This will send IRP_MN_STOP_DEVICE & IRP_MN_START_DEVICE to the underlying device.
                	// To avoid this restarting behavior, just use spDevInstall.Flags|=DI_PROPERTIES_CHANGE .
                
                SetupDiSetDeviceInstallParams(Params->DeviceInfoSet,
                                              Params->DeviceInfoData,
                                              &spDevInstall);
            }
#else
#endif
		}
        return TRUE;

    default:
        return FALSE;
	}}
    return FALSE;   
} 

