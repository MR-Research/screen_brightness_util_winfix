#include "screen_brightness_util_windows_plugin.h"

// This must be included before many other Windows headers.
#include <windows.h>

#include <flutter/method_channel.h>
#include <flutter/plugin_registrar_windows.h>
#include <flutter/standard_method_codec.h>

#include <memory>
#include <sstream>

#include <highlevelmonitorconfigurationapi.h>


using flutter::MethodChannel;

using flutter::EncodableMap;
using flutter::EncodableValue;


namespace screen_brightness_util_windows {

	std::unique_ptr<
		flutter::MethodChannel<flutter::EncodableValue>,
		std::default_delete<flutter::MethodChannel<flutter::EncodableValue>>>
		channel = nullptr;
	std::unique_ptr<ScreenBrightnessUtilWindowsPlugin>  plugin = nullptr;
    HRESULT hr_init = S_OK;  // HRESULT variable for storing function return values

	// static
	void ScreenBrightnessUtilWindowsPlugin::RegisterWithRegistrar(
		flutter::PluginRegistrarWindows* registrar) {
	
		channel =
			std::make_unique<flutter::MethodChannel<flutter::EncodableValue>>(
				registrar->messenger(), "screen_brightness_util",
				&flutter::StandardMethodCodec::GetInstance());
		plugin = std::make_unique<ScreenBrightnessUtilWindowsPlugin>(registrar);

		channel->SetMethodCallHandler(
			[plugin_pointer = plugin.get()](const auto& call, auto result) {
				plugin_pointer->HandleMethodCall(call, std::move(result));
			});
	
		registrar->AddPlugin(std::move(plugin));
	}

	ScreenBrightnessUtilWindowsPlugin::ScreenBrightnessUtilWindowsPlugin(flutter::PluginRegistrarWindows* registrar) : registrar(registrar) {
        // Initialize COM security settings
        hr_init = CoInitializeSecurity(NULL, -1, NULL, NULL,
            RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
            RPC_C_IMP_LEVEL_IMPERSONATE,
            NULL,
            EOAC_SECURE_REFS, //change to EOAC_NONE if you change dwAuthnLevel to RPC_C_AUTHN_LEVEL_NONE
            NULL);
    }

	ScreenBrightnessUtilWindowsPlugin::~ScreenBrightnessUtilWindowsPlugin() {}

	void ScreenBrightnessUtilWindowsPlugin::HandleMethodCall(
		const flutter::MethodCall<flutter::EncodableValue>& method_call,
		std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result) {
		std::string method = method_call.method_name();
	
		//std::cout << "onMethodCall, method:" << method << std::endl;

		if (method_call.method_name().compare("getPlatformVersion") == 0) {
			result->Success(flutter::EncodableValue("10"));
		}
		else if (method.compare("brightness#get") == 0) {
			result->Success(flutter::EncodableValue(getBrightness()));
		}
		else if (method.compare("brightness#set") == 0) {
			auto brightness = *std::get_if<double>(method_call.arguments());
			result->Success(flutter::EncodableValue(setBrightness(brightness)));
		}
		else {
			result->NotImplemented();
		}
	}


	float ScreenBrightnessUtilWindowsPlugin::getBrightness() {

		HMONITOR hMonitor = NULL;
		DWORD cPhysicalMonitors;
		LPPHYSICAL_MONITOR pPhysicalMonitors = NULL;
		HWND hWnd = ::GetAncestor(registrar->GetView()->GetNativeWindow(), GA_ROOT);

		// Get the monitor handle.
		hMonitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTOPRIMARY);
		// Get the number of physical monitors.

		BOOL bSuccess = GetNumberOfPhysicalMonitorsFromHMONITOR(
			hMonitor,
			&cPhysicalMonitors
		);

		if (bSuccess)
		{
			// Allocate the array of PHYSICAL_MONITOR structures.
			pPhysicalMonitors = (LPPHYSICAL_MONITOR)malloc(cPhysicalMonitors * sizeof(PHYSICAL_MONITOR));

			// Get the array.
			bSuccess = GetPhysicalMonitorsFromHMONITOR(
				hMonitor, cPhysicalMonitors, pPhysicalMonitors);

			// Use the monitor handles (not shown).

			HANDLE pmh = pPhysicalMonitors[0].hPhysicalMonitor;
			//SetMonitorBrightness(pmh, 80);
			DWORD  pdwMinimumBrightness = 0;
			DWORD  pdwCurrentBrightness = 0;
			DWORD  pdwMaximumBrightness = 0;

			// attempt to get monitor brightness using WINAPI
			bool winapi_res = GetMonitorBrightness(pmh, &pdwMinimumBrightness, &pdwCurrentBrightness, &pdwMaximumBrightness);
			if (!winapi_res) {
                if (!FAILED(hr_init))
                {
                    // attempt to get monitor brightness using WMI
                    //setNotebookDisplayBrightness_WMI(10);
                    BSTR propToGet1 = SysAllocString(L"CurrentBrightness");
                    BSTR propToGet2 = SysAllocString(L"Levels");
                    BSTR propToGet3 = SysAllocString(L"Level");
                    VARIANT prop1 = getNotebookDisplayProperty_WMI(&propToGet1);
                    VARIANT prop2 = getNotebookDisplayProperty_WMI(&propToGet2);
                    VARIANT prop3 = getNotebookDisplayProperty_WMI(&propToGet3);
                    // get current brightness
                    pdwCurrentBrightness = prop1.intVal;                    
                    // check if result for levels is a proper and valid SAFEARRAY
                    if ((prop3.vt != VT_NULL) && (prop3.vt != VT_EMPTY) && (prop3.vt & VT_ARRAY)) {                        
                        long lLower, lUpper;
                        SAFEARRAY* pSafeArray = prop3.parray;
                        SafeArrayGetLBound(pSafeArray, 1, &lLower);
                        SafeArrayGetUBound(pSafeArray, 1, &lUpper);
                        // get min and max values for brightness
                        pdwMinimumBrightness = static_cast<DWORD>(lLower);
                        pdwMaximumBrightness = static_cast<DWORD>(lUpper);
                        // to get all values from the array
                        //for (long i = lLower; i <= lUpper; i++)
                        //{
                        //    hr = SafeArrayGetElement(pSafeArray, &i, &Element);
                        //    
                        //}
                    }
                }
			}

			DWORD pdwRangeBrightness = pdwMaximumBrightness - pdwMinimumBrightness;
			if (pdwRangeBrightness <= 0) return -1.0;
			float appScreenBrightness = (pdwCurrentBrightness - pdwMinimumBrightness) / (float)pdwRangeBrightness;
			//std::cout << "pdwMinimumBrightness:" << pdwMinimumBrightness << " pdwCurrentBrightness:" << pdwCurrentBrightness << " pdwMaximumBrightness:" << pdwMaximumBrightness << std::endl;

			// Close the monitor handles.
			bSuccess = DestroyPhysicalMonitors(
				cPhysicalMonitors,
				pPhysicalMonitors);

			// Free the array.
			free(pPhysicalMonitors);
			return appScreenBrightness;
		}
		return -1.0;
	}

    // Function to set notebook display brightness using Windows Management Instrumentation (WMI)
    int ScreenBrightnessUtilWindowsPlugin::setNotebookDisplayBrightness_WMI(uint8_t brightness) {

        if (brightness > 100) brightness = 100; // check if brightness value is out of range

        IWbemLocator* pLocator = NULL;   // Pointer to the IWbemLocator interface
        IWbemServices* pNamespace = 0;   // Pointer to the IWbemServices interface
        IWbemClassObject* pClass = NULL; // Pointer to the IWbemClassObject interface
        IWbemClassObject* pInClass = NULL;   // Pointer to the IWbemClassObject interface for input arguments
        IWbemClassObject* pInInst = NULL;    // Pointer to the IWbemClassObject interface for input instance
        IEnumWbemClassObject* pEnum = NULL;  // Pointer to the IEnumWbemClassObject interface for enumerating objects
        HRESULT hr = S_OK;  // HRESULT variable for storing function return values

        BSTR path = SysAllocString(L"root\\wmi");   // Allocate a BSTR and initialize it with the specified string
        BSTR ClassPath = SysAllocString(L"WmiMonitorBrightnessMethods");   // Allocate a BSTR and initialize it with the specified string
        BSTR MethodName = SysAllocString(L"WmiSetBrightness");   // Allocate a BSTR and initialize it with the specified string
        BSTR ArgName0 = SysAllocString(L"Timeout");   // Allocate a BSTR and initialize it with the specified string
        BSTR ArgName1 = SysAllocString(L"Brightness");   // Allocate a BSTR and initialize it with the specified string
        BSTR bstrQuery = SysAllocString(L"Select * from WmiMonitorBrightnessMethods");   // Allocate a BSTR and initialize it with the specified string


        // Checks if any of the variables path, ClassPath, MethodName, or ArgName0 is null
        if (!path || !ClassPath || !MethodName || !ArgName0)
        {
            printf("SysAllocString failed. Out of memory.\n");
            goto cleanup;
        }

        // Initialize Component Object Model (COM)
        hr = CoInitialize(0);
        if (FAILED(hr))
        {
            printf("CoInitialize returned 0x%x:", hr);
            goto cleanup;
        }

        if (FAILED(hr_init))
        {
            printf("CoInitializeSecurity returned 0x%x:", hr);
            goto cleanup;
        }

        // Create an instance of the WbemLocator object
        hr = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER,
            IID_IWbemLocator, (LPVOID*)&pLocator);
        if (FAILED(hr))
        {
            printf("CoCreateInstance returned 0x%x:", hr);
            goto cleanup;
        }

        // Connect to the WMI server
        hr = pLocator->ConnectServer(path, NULL, NULL, NULL, 0, NULL, NULL, &pNamespace);
        printf("\n\nConnectServer returned 0x%x:", hr);
        if (hr != WBEM_S_NO_ERROR)
            goto cleanup;

        // Set the security blanket on the proxy
        hr = CoSetProxyBlanket(pNamespace,
            RPC_C_AUTHN_WINNT,
            RPC_C_AUTHZ_NONE,
            NULL,
            RPC_C_AUTHN_LEVEL_PKT,
            RPC_C_IMP_LEVEL_IMPERSONATE,
            NULL,
            EOAC_NONE
        );

        if (hr != WBEM_S_NO_ERROR)
            goto cleanup;

        // Execute a query on the namespace
        hr = pNamespace->ExecQuery(_bstr_t(L"WQL"), //Query Language
            bstrQuery, //Query to Execute
            WBEM_FLAG_RETURN_IMMEDIATELY, //Make a semi-synchronous call
            NULL, //Context
            &pEnum //Enumeration Interface
        );

        if (hr != WBEM_S_NO_ERROR)
            goto cleanup;

        hr = WBEM_S_NO_ERROR;

        // Iterate through the objects returned by the query
        while (WBEM_S_NO_ERROR == hr)
        {
            // Get the next object from the collection
            ULONG ulReturned;
            IWbemClassObject* pObj;
            //DWORD retVal = 0;

            hr = pEnum->Next(WBEM_INFINITE, //Timeout
                1, //No of objects requested
                &pObj, //Returned Object
                &ulReturned //No of object returned
            );

            if (hr != WBEM_S_NO_ERROR)
                goto cleanup;

            // Get the class object
            hr = pNamespace->GetObject(ClassPath, 0, NULL, &pClass, NULL);
            printf("\nGetObject returned 0x%x:", hr);
            if (hr != WBEM_S_NO_ERROR)
                goto cleanup;

            // Get the input argument and set the property
            hr = pClass->GetMethod(MethodName, 0, &pInClass, NULL);
            printf("\nGetMethod returned 0x%x:", hr);
            if (hr != WBEM_S_NO_ERROR)
                goto cleanup;

            // Create an instance of the input argument class
            hr = pInClass->SpawnInstance(0, &pInInst);
            printf("\nSpawnInstance returned 0x%x:", hr);
            if (hr != WBEM_S_NO_ERROR)
                goto cleanup;

            // Set the value of the first argument (timeout)
            VARIANT var1;
            VariantInit(&var1);
            V_VT(&var1) = VT_BSTR;
            V_BSTR(&var1) = SysAllocString(L"0");
            hr = pInInst->Put(ArgName0, 0, &var1, CIM_UINT32); //CIM_UINT64
            printf("\nPut ArgName0 returned 0x%x:", hr);
            VariantClear(&var1);
            if (hr != WBEM_S_NO_ERROR)
                goto cleanup;

            // Set the value of the second argument (brightness)
            VARIANT var;
            VariantInit(&var);
            var.vt = VT_UI1;
            var.uiVal = brightness;
            hr = pInInst->Put(ArgName1, 0, &var, 0);
            VariantClear(&var);
            printf("\nPut ArgName1 returned 0x%x:", hr);
            if (hr != WBEM_S_NO_ERROR)
                goto cleanup;

            // Call the method
            VARIANT pathVariable;
            VariantInit(&pathVariable);
            hr = pObj->Get(_bstr_t(L"__PATH"), 0, &pathVariable, NULL, NULL);
            printf("\npObj Get returned 0x%x:", hr);
            if (hr != WBEM_S_NO_ERROR)
                goto cleanup;

            hr = pNamespace->ExecMethod(pathVariable.bstrVal, MethodName, 0, NULL, pInInst, NULL, NULL);
            VariantClear(&pathVariable);
            printf("\nExecMethod returned 0x%x:", hr);
            if (hr != WBEM_S_NO_ERROR)
                goto cleanup;
        }

        printf("Terminating normally\n");


        // Free up resources
    cleanup:
        SysFreeString(path);   // Free the BSTR
        SysFreeString(ClassPath);   // Free the BSTR
        SysFreeString(MethodName);   // Free the BSTR
        SysFreeString(ArgName0);   // Free the BSTR
        SysFreeString(ArgName1);   // Free the BSTR
        SysFreeString(bstrQuery);   // Free the BSTR

        if (pClass)
            pClass->Release();   // Release the IWbemClassObject interface
        if (pInInst)
            pInInst->Release();   // Release the IWbemClassObject interface
        if (pInClass)
            pInClass->Release();   // Release the IWbemClassObject interface
        if (pLocator)
            pLocator->Release();   // Release the IWbemLocator interface
        if (pNamespace)
            pNamespace->Release();   // Release the IWbemServices interface
        CoUninitialize();   // Uninitialize the COM library
        return 0;   // Return 0 to indicate successful execution
    }

    VARIANT ScreenBrightnessUtilWindowsPlugin::getNotebookDisplayProperty_WMI(BSTR *PropertyName) {

        IWbemLocator* pLocator = NULL;   // Pointer to the IWbemLocator interface
        IWbemServices* pNamespace = 0;   // Pointer to the IWbemServices interface
        IEnumWbemClassObject* pEnum = NULL;  // Pointer to the IEnumWbemClassObject interface for enumerating objects
        HRESULT hr = S_OK;  // HRESULT variable for storing function return values

        BSTR path = SysAllocString(L"root\\wmi");   // Allocate a BSTR and initialize it with the specified string
        BSTR ClassPath = SysAllocString(L"WmiMonitorBrightness");   // Allocate a BSTR and initialize it with the specified string
        //BSTR PropertyName = SysAllocString(L"CurrentBrightness");   // Allocate a BSTR and initialize it with the specified string
        BSTR bstrQuery = SysAllocString(L"Select * from WmiMonitorBrightness");   // Allocate a BSTR and initialize it with the specified string
        VARIANT objProp;
        VariantInit(&objProp);

        // Checks if any of the variables path, ClassPath, MethodName, or ArgName0 is null
        if (!path || !ClassPath)
        {
            printf("SysAllocString failed. Out of memory.\n");
            goto cleanup;
        }

        // Initialize Component Object Model (COM)
        hr = CoInitialize(0);
        if (FAILED(hr))
        {
            printf("CoInitialize returned 0x%x:", hr);
            goto cleanup;
        }

        // Create an instance of the WbemLocator object
        hr = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER,
            IID_IWbemLocator, (LPVOID*)&pLocator);
        if (FAILED(hr))
        {
            printf("CoCreateInstance returned 0x%x:", hr);
            goto cleanup;
        }

        // Connect to the WMI server
        hr = pLocator->ConnectServer(path, NULL, NULL, NULL, 0, NULL, NULL, &pNamespace);
        printf("\n\nConnectServer returned 0x%x:", hr);
        if (hr != WBEM_S_NO_ERROR)
            goto cleanup;

        // Set the security blanket on the proxy
        hr = CoSetProxyBlanket(pNamespace,
            RPC_C_AUTHN_WINNT,
            RPC_C_AUTHZ_NONE,
            NULL,
            RPC_C_AUTHN_LEVEL_PKT,
            RPC_C_IMP_LEVEL_IMPERSONATE,
            NULL,
            EOAC_NONE
        );

        if (hr != WBEM_S_NO_ERROR)
            goto cleanup;

        // Execute a query on the namespace
        hr = pNamespace->ExecQuery(_bstr_t(L"WQL"), //Query Language
            bstrQuery, //Query to Execute
            WBEM_FLAG_RETURN_IMMEDIATELY, //Make a semi-synchronous call
            NULL, //Context
            &pEnum //Enumeration Interface
        );

        if (hr != WBEM_S_NO_ERROR)
            goto cleanup;

        hr = WBEM_S_NO_ERROR;
        //int retVal = 0;
        
        // Iterate through the objects returned by the query
        while (WBEM_S_NO_ERROR == hr)
        {
            // Get the next object from the collection
            ULONG ulReturned;
            IWbemClassObject* pObj;
            

            hr = pEnum->Next(WBEM_INFINITE, //Timeout
                1, //No of objects requested
                &pObj, //Returned Object
                &ulReturned //No of object returned
            );

            if (hr != WBEM_S_NO_ERROR)
                goto cleanup;

            // Call the method            
            hr = pObj->Get(*PropertyName, 0, &objProp, NULL, NULL);
            printf("\npObj Get returned 0x%x:", hr);
            if (hr != WBEM_S_NO_ERROR)
                goto cleanup;
            
        }

        printf("Terminating normally\n");
        return objProp;
        // Free up resources
    cleanup:
        SysFreeString(path);   // Free the BSTR
        SysFreeString(ClassPath);   // Free the BSTR
        SysFreeString(bstrQuery);   // Free the BSTR

        if (pLocator)
            pLocator->Release();   // Release the IWbemLocator interface
        if (pNamespace)
            pNamespace->Release();   // Release the IWbemServices interface
        CoUninitialize();   // Uninitialize the COM library
        return objProp;   // Return 0 to indicate successful execution
    }

	bool ScreenBrightnessUtilWindowsPlugin::setBrightness(const double brightness) {

		HMONITOR hMonitor = NULL;
		DWORD cPhysicalMonitors;
		LPPHYSICAL_MONITOR pPhysicalMonitors = NULL;
		HWND hWnd = ::GetAncestor(registrar->GetView()->GetNativeWindow(), GA_ROOT);

		// Get the monitor handle.
		hMonitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTOPRIMARY);
		// Get the number of physical monitors.

		BOOL bSuccess = GetNumberOfPhysicalMonitorsFromHMONITOR(
			hMonitor,
			&cPhysicalMonitors
		);

		if (bSuccess)
		{
			// Allocate the array of PHYSICAL_MONITOR structures.
			pPhysicalMonitors = (LPPHYSICAL_MONITOR)malloc(cPhysicalMonitors * sizeof(PHYSICAL_MONITOR));


			// Get the array.
			bSuccess = GetPhysicalMonitorsFromHMONITOR(
				hMonitor, cPhysicalMonitors, pPhysicalMonitors);

			// Use the monitor handles (not shown).

			HANDLE pmh = pPhysicalMonitors[0].hPhysicalMonitor;
		
			DWORD  pdwMinimumBrightness = 0;
			DWORD  pdwCurrentBrightness = 0;
			DWORD  pdwMaximumBrightness = 0;

            // attempt to get monitor brightness using WINAPI
            bool winapi_res = GetMonitorBrightness(pmh, &pdwMinimumBrightness, &pdwCurrentBrightness, &pdwMaximumBrightness);
            if (!winapi_res) {
                if (!FAILED(hr_init))
                {
                    // attempt to get monitor brightness using WMI
                    //setNotebookDisplayBrightness_WMI(10);
                    //BSTR propToGet1 = SysAllocString(L"CurrentBrightness");
                    BSTR propToGet2 = SysAllocString(L"Levels");
                    BSTR propToGet3 = SysAllocString(L"Level");
                    //VARIANT prop1 = getNotebookDisplayProperty_WMI(&propToGet1);
                    VARIANT prop2 = getNotebookDisplayProperty_WMI(&propToGet2);
                    VARIANT prop3 = getNotebookDisplayProperty_WMI(&propToGet3);
                    // get current brightness
                    //pdwCurrentBrightness = prop1.intVal;
                    // check if result for levels is a proper and valid SAFEARRAY
                    if ((prop3.vt != VT_NULL) && (prop3.vt != VT_EMPTY) && (prop3.vt & VT_ARRAY)) {
                        long lLower, lUpper;
                        SAFEARRAY* pSafeArray = prop3.parray;
                        SafeArrayGetLBound(pSafeArray, 1, &lLower);
                        SafeArrayGetUBound(pSafeArray, 1, &lUpper);
                        // get min and max values for brightness
                        pdwMinimumBrightness = static_cast<DWORD>(lLower);
                        pdwMaximumBrightness = static_cast<DWORD>(lUpper);
                        // to get all values from the array
                        //for (long i = lLower; i <= lUpper; i++)
                        //{
                        //    hr = SafeArrayGetElement(pSafeArray, &i, &Element);
                        //    
                        //}
                    }
                }
            }


			DWORD pdwRangeBrightness = pdwMaximumBrightness - pdwMinimumBrightness;
			DWORD brightnessFinal = (DWORD)(pdwRangeBrightness * brightness + pdwMinimumBrightness);
            // set monitor brightness using WINAPI
            winapi_res = SetMonitorBrightness(pmh, brightnessFinal);
			//std::cout << "brightness:" << brightness << ", brightnessFinal:" << brightnessFinal << std::endl;
            // set monitor brightness using WMI
            if (!winapi_res) {
                setNotebookDisplayBrightness_WMI(static_cast<uint8_t>(brightnessFinal));
            }

			// Close the monitor handles.
			bSuccess = DestroyPhysicalMonitors(
				cPhysicalMonitors,
				pPhysicalMonitors);

			// Free the array.
			free(pPhysicalMonitors);
			return true;
		}
		return false;
	}
}  // namespace screen_brightness_util_windows
