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
//              be restarted to apply the new attributes. // ���ע�������Ѿ���ʱ��,ȴû��ɾ��

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
#include <assert.h>
#include <stdio.h>
#include "resource.h"

#ifndef DONT_USE_WDK 
#include <dontuse.h> // E:\WinDDK\7600.16385.1\inc\api\dontuse.h
#endif

//+---------------------------------------------------------------------------
//
// WARNING! 
//
// Installer must not generate any popup to the user.
//    It should provide appropriate defaults.
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


INT_PTR
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
    Entry-point for adding additional device manager property  sheet pages.  

Returns:    NO_ERROR, ERROR_DI_DO_DEFAULT, or an error code.

������Ӧ����ѭ�Ĵ�������� WDK7 chm ���� "Handling DIF_ADDPROPERTYPAGE_ADVANCED Requests"
��������ַ(2016-08-28) https://msdn.microsoft.com/windows/hardware/drivers/install/creating-custom-property-pages

ִ��ʱ��: �ոմ��豸�������� ToasterDevice01 ���ԶԻ���ʱ, �˺����ͻᱻִ��.
--*/
{
    HPROPSHEETPAGE  pageHandle;
    PROPSHEETPAGE   page;
    PTOASTER_PROP_PARAMS      params = NULL;
    SP_ADDPROPERTYPAGE_DATA AddPropertyPageData = {0};

    //
    // DeviceInfoSet is NULL if setup is requesting property pages for
    // the device setup class. We don't want to do anything in this 
    // case.
    //
    if (DeviceInfoData==NULL) {
        return ERROR_DI_DO_DEFAULT;
    }

    AddPropertyPageData.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);

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
        params = HeapAlloc(GetProcessHeap(), 0, sizeof(TOASTER_PROP_PARAMS));
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
                AddPropertyPageData.NumDynamicPages++]=pageHandle;

            SetupDiSetClassInstallParams(DeviceInfoSet,
                        DeviceInfoData,
                        (PSP_CLASSINSTALL_HEADER)&AddPropertyPageData,
                        sizeof(SP_ADDPROPERTYPAGE_DATA));
        }
    }
    return NO_ERROR;
} 

INT_PTR
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
		// ���û���� "Custom Property Page" ѡ�ʱ�Ż�ײ��, 
		// ������ ToasterDevice ���ԶԻ��򲢲���ײ��.

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
		// �ر� ToasterDevice ���ԶԻ���ʱײ��.
		DbgOut("PropPageDlgProc.WM_DESTROY"); // chj debug

    default: 
        return FALSE;
	}}

    return TRUE;
} 


UINT
CALLBACK
PropPageDlgCallback(HWND hwnd,
                   UINT uMsg,
                   LPPROPSHEETPAGE ppsp)
{
    PTOASTER_PROP_PARAMS params;
    UNREFERENCED_PARAMETER( hwnd );

    switch (uMsg) 
	{{
    case PSPCB_CREATE:
        // ����һ�ε�� "Custom Property Page" ѡ�ʱ, ��ײ���˴�, 
		// ���� PropPageDlgProc.WM_INITDIALOG ֮ǰײ��.
		//
        // Called when the property sheet is first displayed
        //
        return TRUE;    // return TRUE to continue with creation of page

    case PSPCB_RELEASE:
		// �� PropPageDlgProc.WM_DESTROY ֮��ײ��.
		// ��ע��: ����û�û��ȥ���"Custom Property Page" ѡ�, WM_INITDIALOG �� WM_DESTROY �����ᱻִ��.
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

            //
            // Inform setup about property change so that it can
            // restart the device.
            //

            spDevInstall.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
     
            if (Params && SetupDiGetDeviceInstallParams(Params->DeviceInfoSet,
                                              Params->DeviceInfoData,
                                              &spDevInstall)) 
			{
                // If your device requires a reboot to restart, you can
                // specify that by setting DI_NEEDREBOOT as shown below
                //
                // if(NeedReboot) {
                //    spDevInstall.Flags |= DI_PROPERTIES_CHANGE | DI_NEEDREBOOT;
                // }
                //
                spDevInstall.FlagsEx |= DI_FLAGSEX_PROPCHANGE_PENDING;
                
                SetupDiSetDeviceInstallParams(Params->DeviceInfoSet,
                                              Params->DeviceInfoData,
                                              &spDevInstall);
            }
        }
        return TRUE;

    default:
        return FALSE;
	}}
    return FALSE;   
} 

