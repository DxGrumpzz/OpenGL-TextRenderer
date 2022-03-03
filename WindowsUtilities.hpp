#pragma once

#pragma warning(push)

// Ignore warning C4505 unreferenced function with internal linkage has been removed.
// Some function which are marked as static, will not be part of the final executable. Therefore, they will be removed, this is fine.
#pragma warning(disable: 4505)

// Ignore warning 4211 nonstandard extension used: redefined extern to static.
// Due to the way this file is structured, with the namespaces and all. We have to forward declare some function to properly use them.
#pragma warning(disable: 4211)


#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <comdef.h>
#include <string>
#include <memory>
#include <cstdint>
#include <gdiplus.h>
#include <vector>
#include <bcrypt.h>
#include <shellapi.h>
#include <locale>
#include <codecvt>
#include <winsock.h>
#include <chrono>
#include <functional>
#include <thread>
#include <cassert>
#include <source_location>
#include <filesystem>


#pragma comment(lib, "Bcrypt.lib")


#pragma region // Forward declarations, don't you just *love* C ?

// Note: This order of:
//   namespace 
//   alias
//   namesapce 
// Is REALLY important

namespace WindowsUtilities
{
    std::wstring GetLastErrorAsStringW();
};

namespace WinUtils = WindowsUtilities;
namespace winutils = WinUtils;
namespace wt = winutils;

namespace WindowsUtilities
{
    namespace Debug
    {
        static void DebugBreakIfAttached();
    };
    namespace dbg = Debug;
};

#pragma endregion


// Main utilities and tools
namespace WindowsUtilities
{

    /// <summary>
    /// A simple wrapper class around a Win32 HANDLE
    /// </summary>
    class SmartWin32Handle
    {
    private:

        /// <summary>
        /// The main handle used to comminucate with the WinAPI
        /// </summary>
        HANDLE _handle = nullptr;


    public:

        SmartWin32Handle(HANDLE handle) :
            _handle(handle)
        {
        };


        ~SmartWin32Handle()
        {
            // Close the handle using windows defined functions when 
            // destructor is invoked

            if (_handle == nullptr)
                return;

            CloseHandle(_handle);
        };


    public:

        const HANDLE& Get()
        {
            return _handle;
        };


    public:

        SmartWin32Handle& operator = (const HANDLE& handle)
        {
            _handle = handle;

            return *this;
        };

        /// <summary>
        /// Implicit cast to WIN32 handle
        /// (I like the syntax when passing to functions)
        /// </summary>
        operator HANDLE()
        {
            return _handle;
        };

    };


    /// <summary>
    /// A wrapper class around an HMODULE.
    /// Allows for library loading, function loading, and automatic cleanup of loaded libraries.
    /// </summary>
    class SmartWin32Library
    {

    private:

        /// <summary>
        /// The name of the loaded DLL
        /// </summary>
        const std::wstring _libraryName;

        /// <summary>
        /// A handle to the loaded DLL
        /// </summary>
        HMODULE _library = nullptr;


    public:

        SmartWin32Library(const std::wstring& libraryName) :
            _libraryName(libraryName)
        {
            if (libraryName.empty() == true)
                throw std::exception("DLL name cannot be empty");

            // Load the DLL
            _library = LoadLibraryW(libraryName.data());

            if (_library == nullptr)
                throw std::exception("Cannot load DLL");
        };


        ~SmartWin32Library()
        {
            // c l e a n u p
            if (_library != nullptr)
                FreeLibrary(_library);
        };


    public:


        /// <summary>
        /// Returns a function from a loaded DLL
        /// </summary>
        /// <param name="functionName"> The name of the function as it appears inside the export list of the DLL </param>
        /// <typeparam name="TSignature"> The function's signature </typeparam>
        /// <returns></returns>
        template<typename TSignature>
        std::function<TSignature> GetLibraryFunction(const std::string& functionName) const
        {
            if (functionName.empty() == true)
                throw std::exception("Function name cannot be empty");

            if (_library == nullptr)
                throw std::exception("Library handle is null");


            // Find the function in the DLL
            FARPROC dllFunctionAddress = GetProcAddress(_library, functionName.data());

            // Convert the function pointer into a valid 'std::function'
            return reinterpret_cast<TSignature*>(dllFunctionAddress);
        };


    public:


        HMODULE GetLibrary() const
        {
            return _library;
        };

        const HMODULE GetLibraryConst() const
        {
            return GetLibrary();
        };

    };



    #pragma region General utility functions

    /// <summary>
    /// Takes a wide string command line argument and converts it to a vector of split wide strings
    /// </summary>
    /// <param name="commandLineString"> The command line argument string, recommended to use GetCommnadLineW() </param>
    /// <returns></returns>
    static std::vector<std::wstring> CommandLineToVectorW(const std::wstring& commandLineString)
    {
        // The number of available command line arguments
        std::size_t argsCount = 0;

        // CommandLineToArgvW converts a command line string literal, to a "tokenized" array of strings
        const wchar_t** arg = const_cast<const wchar_t**>(CommandLineToArgvW(commandLineString.c_str(), reinterpret_cast<int*>(&argsCount)));

        // A vector that will store the split args
        std::vector<std::wstring> commandLineVector;
        commandLineVector.reserve(argsCount);

        // Copy the tokenized arguments into commandLineVector
        for (std::size_t i = 0; i < argsCount; i++)
        {
            const std::size_t argLength = std::wcslen(arg[i]);

            commandLineVector.emplace_back(arg[i], arg[i] + argLength);
        };

        return commandLineVector;
    };


    /// <summary>
    /// Takes a narrow string command line argument and converts it to a vector of split narrow strings 
    /// </summary>
    /// <param name="commandLineString"> The command line argument string, recommended to use GetCommnadLineA() </param>
    /// <returns></returns>
    static std::vector<std::string> CommandLineToVectorA(const std::string& commandLineString)
    {
        // A temporary buffer that will store the narrow command line string as wide string
        std::wstring commandLineStringW = std::wstring(commandLineString.cbegin(), commandLineString.cend());

        // Convert the narrow command line into a wide string

        // Split the command line string into tokenized arguments
        const std::vector<std::wstring> commandLineVectorW = CommandLineToVectorW(commandLineStringW);


        // A vector that will store the narrow string command line arguments
        std::vector<std::string> commandLineVectorA;
        commandLineVectorA.reserve(commandLineVectorW.size());


        // Convert, and copy (in that order) the wide string into commandLineVectorA
        for (std::size_t i = 0; i < commandLineVectorW.size(); i++)
        {
            const std::wstring& argW = commandLineVectorW[i];

            // A temporary narrow string that will hold the converted characters
            std::string argA;
            argA.resize(argW.size());

            std::size_t charactersConverted = 0;
            wcstombs_s(&charactersConverted, &argA[0], argA.size() + 1, argW.c_str(), argW.size());

            commandLineVectorA.emplace_back(std::move(argA));
        };

        return commandLineVectorA;
    };


    /// <summary>
    /// Simple shorthand way to call MessageBox, ommits 1st parameter by passing nullptr as the default value
    /// </summary>
    /// <param name="text"></param>
    /// <param name="title"></param>
    /// <param name="icon"></param>
    /// <returns></returns>
    static int MessageBoxA(const char* text, const char* title = "", std::uint32_t icon = 0)
    {
        return ::MessageBoxA(nullptr, text, title, icon);
    };

    /// <summary>
    /// Simple shorthand way to call MessageBox, ommits 1st parameter by passing nullptr as the default value
    /// </summary>
    /// <param name="text"></param>
    /// <param name="title"></param>
    /// <param name="icon"></param>
    /// <returns></returns>
    static int MessageBoxW(const wchar_t* text, const wchar_t* title, std::uint32_t icon = 0)
    {
        return ::MessageBoxW(nullptr, text, title, icon);
    };



    #undef MessageBox
    static int MessageBox(
        #ifdef UNICODE
        const wchar_t* text,
        const wchar_t* title = L"",
        #else
        const char* text,
        const char* title = "",
        #endif
        const std::uint32_t icon = 0
    )
    {
        #ifdef UNICODE
        return winutils::MessageBoxW(text, title, icon);
        #else
        return winutils::MessageBoxA(text, title, icon);
        #endif
};
    #ifndef MessageBox
    #endif


    /// <summary>
    /// Takes a DWORD error code and returns its string message  
    /// </summary>
    /// <param name="error"></param>
    /// <returns></returns>
    static std::wstring ErrorToStringW(DWORD error)
    {
        // Stores the error message as a string in memory
        LPWSTR tempBuffer = nullptr;

        // Format DWORD error ID to a string 
        std::uint32_t bufferLength = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                                    nullptr,
                                                    error,
                                                    static_cast<std::uint16_t>(0x0409),
                                                    (LPWSTR)&tempBuffer,
                                                    0,
                                                    nullptr);

        // Copy the contents of the buffer into a wstring
        std::wstring errorString(tempBuffer, tempBuffer + bufferLength);

        // Free the allocated buffer
        LocalFree(tempBuffer);

        return errorString;
    };

    /// <summary>
    /// Takes a DWORD error code and returns its string message  
    /// </summary>
    /// <param name="error"></param>
    /// <returns></returns>
    static std::string ErrorToStringA(DWORD error)
    {
        // Stores the error message as a string in memory
        LPSTR tempBuffer = nullptr;

        // Format DWORD error ID to a string 
        std::uint32_t bufferLength = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                                    NULL,
                                                    error,
                                                    static_cast<std::uint16_t>(0x0409),
                                                    (LPSTR)&tempBuffer, 0, NULL);

        // Copy the contents of the buffer into a wstring
        std::string errorString(tempBuffer, tempBuffer + bufferLength);

        // Create std string from buffer
        return errorString;
    };

    /// <summary>
    /// Convert an NT error code to a string representation
    /// </summary>
    /// <param name="ntError"> The NT error code </param>
    /// <returns></returns>
    static std::wstring NTErrorToStringW(NTSTATUS ntError)
    {
        // Stores the error message as a string in memory
        LPWSTR tempBuffer = nullptr;

        // Load the NTDLL library where Windows keeps all the NT error codes
        const HMODULE ntDllHandle = LoadLibraryW(L"NTDLL.dll");

        // If no handle was found for the NTDLL library
        if (ntDllHandle == nullptr)
        {
            // Show error message
            const std::wstring error = GetLastErrorAsStringW();
            WinUtils::dbg::DebugBreakIfAttached();
            return error;
        };

        // Format DWORD error ID to a string 
        std::uint32_t bufferLength = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_IGNORE_INSERTS,
                                                    ntDllHandle,
                                                    ntError,
                                                    static_cast<std::uint16_t>(0x0409),
                                                    (LPWSTR)&tempBuffer,
                                                    0,
                                                    nullptr);

        // Copy the contents of the buffer into a wstring
        std::wstring errorString(tempBuffer, tempBuffer + bufferLength);

        // Free the NT library
        FreeLibrary(ntDllHandle);

        // Free the allocated buffer
        LocalFree(tempBuffer);

        return errorString;
    };


    /// <summary>
    /// Returns the last windows error as a wide string
    /// </summary>
    /// <returns></returns>
    static std::wstring GetLastErrorAsStringW()
    {
        return ErrorToStringW(GetLastError());
    };

    /// <summary>
    /// Returns the last windows error as a ansi-string
    /// </summary>
    /// <returns></returns>
    static std::string GetLastErrorAsStringA()
    {
        return ErrorToStringA(GetLastError());
    };



    /// <summary>
    /// A function that displays a MessageBox with some error details
    /// </summary>
    /// <param name="error"> The error to display </param>
    /// <param name="title"> The error title </param>
    /// <param name="line"> Current line number, ideally do not pass a value here </param>
    /// <param name="file"> Current file, ideally do not pass a value here</param>
    static void ShowWinErrorW(const std::wstring& error, const std::wstring& title = L"WinError",
                              std::intmax_t line = __LINE__, const wchar_t* file = __FILEW__)
    {
        // Create a digestable error message
        std::wstring outputError;
        outputError.reserve(256);

        outputError.append(L"An error occured in ")
            .append(file)
            .append(L"\n")
            .append(L"Line: ")
            .append(std::to_wstring(line))
            .append(L"\n")
            .append(L"Error:\n")
            .append(error);

        MessageBoxW(outputError.c_str(), title.c_str(), MB_ICONERROR);

    };


    /// <summary>
    /// A function that displays a MessageBox with some error details
    /// </summary>
    /// <param name="error"> The error to display </param>
    /// <param name="title"> The error title </param>
    /// <param name="line"> Current line number, ideally do not pass a value here </param>
    /// <param name="file"> Current file, ideally do not pass a value here</param>
    static void ShowWinErrorA(const std::string& error, const std::string& title = "WinError",
                              std::intmax_t line = __LINE__, const char* file = __FILE__)
    {
        // Create a digestable error message
        std::string outputError;
        outputError.reserve(256);

        outputError.append("An error occured in ")
            .append(file)
            .append("\n")
            .append("Line: ")
            .append(std::to_string(line))
            .append("\n")
            .append("Error:\n")
            .append(error);

        MessageBoxA(outputError.c_str(), title.c_str(), MB_ICONERROR);

    };


    #pragma warning(push)
    // Ignore unused parameter warning
    #pragma warning(disable: 4100)
    /// <summary>
    /// A function that will assert an expression. 
    /// Will throw CRT exception if expression results in "false"
    /// </summary>
    /// <param name="expression"> The expression to assert </param>
    /// <param name="message"> A message that will be dispalyed in the CRT dialog if the expression fails </param>
    /// <param name="sourceLocation"> The location at which this function was called  </param>
    static bool Assert(const bool expression, const std::string_view& message, const std::source_location sourceLocation = std::source_location::current())
    {
        #ifdef _DEBUG

        if(expression == false)
        {
            char modulePath[MAX_PATH + 1] { 0 };
            GetModuleFileNameA(nullptr, modulePath, MAX_PATH + 1);

            const std::string moduleFilename = std::filesystem::path(modulePath).filename().string();

            _CrtDbgReport(_CRT_ASSERT, sourceLocation.file_name(), sourceLocation.line(), moduleFilename.c_str(), "%s", message.data());
        };

        return expression;
        #else
        return true;
        #endif
    };

    /// <summary>
    /// A function that will assert an expression. 
    /// Will throw CRT exception if expression results in "false"
    /// </summary>
    /// <param name="expression"> The expression to assert </param>
    /// <param name="messageResult"> A callable function that will return a message as a "std::string". Will only be called if the expression fails </param>
    /// <param name="sourceLocation"> The location at which this function was called  </param>
    static bool Assert(const bool expression, const std::function<std::string(void)>& messageResult, const std::source_location sourceLocation = std::source_location::current())
    {
        #ifdef _DEBUG

        if(expression == false)
        {
            char modulePath[MAX_PATH + 1] { 0 };
            GetModuleFileNameA(nullptr, modulePath, MAX_PATH + 1);

            const std::string moduleFilename = std::filesystem::path(modulePath).filename().string();

            _CrtDbgReport(_CRT_ASSERT, sourceLocation.file_name(), sourceLocation.line(), moduleFilename.c_str(), "%s", messageResult().data());
        };


        return expression;
        #else
        return true;
        #endif
    };

    #pragma warning(pop)


    #pragma endregion


    #pragma region Windows API debug calls

    #ifdef _DEBUG
    #define WINCALL(result) WinCall(result, __LINE__, __FILEW__)
    #else
    #define WINCALL(result) WinCall(result, __LINE__, __FILEW__)
    #endif

    #define REGCALL(result) RegCall(result, __LINE__, __FILEW__)

    #ifndef NT_SUCCESS
    #define NT_SUCCESS(Status) ((NTSTATUS)(Status) >= 0)
    #endif

    static std::intmax_t WinCall(std::intmax_t result, std::intmax_t line, const wchar_t* file)
    {
        if (!result)
        {
            ShowWinErrorW(GetLastErrorAsStringW(), L"WinError", line, file);

            DebugBreak();
        };

        return result;
    };

    inline HANDLE WinCall(HANDLE handleResult, std::intmax_t line, const wchar_t* file)
    {
        if (handleResult == NULL)
        {
            ShowWinErrorW(GetLastErrorAsStringW(), L"WinError", line, file);

            DebugBreak();
        };

        return handleResult;
    };

    inline HCURSOR WinCall(HCURSOR hCursorResult, std::intmax_t line, const wchar_t* file)
    {
        if (hCursorResult == NULL)
        {
            ShowWinErrorW(GetLastErrorAsStringW(), L"WinError", line, file);

            DebugBreak();
        };

        return hCursorResult;
    };

    inline HWND WinCall(HWND hwnd, std::intmax_t line, const wchar_t* file)
    {
        if (hwnd == NULL)
        {
            ShowWinErrorW(GetLastErrorAsStringW(), L"WinError", line, file);

            DebugBreak();
        };

        return hwnd;
    };

    inline HMODULE WinCall(HMODULE hModule, std::intmax_t line, const wchar_t* file)
    {
        if (hModule == NULL)
        {
            ShowWinErrorW(GetLastErrorAsStringW(), L"WinError", line, file);

            DebugBreak();
        };

        return hModule;
    };

    inline NTSTATUS WinCall(const NTSTATUS ntStatus, const std::intmax_t line, const wchar_t* file)
    {
        if (!NT_SUCCESS(ntStatus))
        {
            ShowWinErrorW(NTErrorToStringW(ntStatus), L"WinError", line, file);
        };

        return ntStatus;
    };

    inline LSTATUS RegCall(const LSTATUS lStatus, const std::intmax_t line, const wchar_t* file)
    {
        if (lStatus != ERROR_SUCCESS)
        {
            ShowWinErrorW(ErrorToStringW(lStatus), L"WinError", line, file);
        };

        return lStatus;
    };

    inline WORD WinCall(const WORD result, const std::intmax_t line, const wchar_t* file)
    {
        if (!result)
        {
            ShowWinErrorW(NTErrorToStringW(result), L"WinError", line, file);
        };

        return result;
    };

    inline int WinCall(const int result, const std::intmax_t line, const wchar_t* file)
    {
        if (!result)
        {
            ShowWinErrorW(NTErrorToStringW(result), L"WinError", line, file);
        };

        return result;
    };

    #pragma endregion


    #pragma region WinSock calls

    #define WINSOCK(result) _WINSOCK(result, __LINE__, __FILEW__)

    inline int _WINSOCK(int result, std::intmax_t line, const wchar_t* file)
    {
        if (result != 0)
        {
            std::wstring errorString;

            errorString.append(L"Socket error.\n")
                .append(ErrorToStringW(WSAGetLastError()))
                .append(L"Error code: ")
                .append(std::to_wstring(WSAGetLastError()));

            ShowWinErrorW(errorString, L"Socket error", line, file);

            throw std::exception("Socket error");
        };

        return result;
    };

    #pragma endregion


    #pragma region GDI+ calls

    // Wraps a call to a GDI+ function and displays any errors
    #define GDICALL(statusResult) _GDICALL_(statusResult, __LINE__, __FILEW__)


    inline void _GDICALL_(Gdiplus::Status status, std::uint32_t line, const wchar_t* file)
    {

        if (status != Gdiplus::Status::Ok)
        {
            std::wstring outputErrorString;

            ShowWinErrorW(L"A GDI+ error has occured.", L"GDI+ Error", line, file);
            DebugBreak();
        };

    };

    #pragma endregion


    #pragma region COM debug calls

    // _com_error struct uses 'Hard-coded' unicode or ANSI function depending on the environment.
    // So I must check before compiling what environemnt the project is currently using
    #ifdef UNICODE

    // A macro used to debug COM functions.
    // Creates a scoped variable of type HRESULT and stores the retured function value,
    // using COM's FAILED macro, checks if the result is valid or not
    #define COMCALL(function) _COM_CALL(function, __LINE__, __FILEW__)

    inline HRESULT _COM_CALL(HRESULT hResult,
                             std::intmax_t line, const wchar_t* file)
    {
        if (FAILED(hResult))
        {
            _com_error comError(hResult);

            ShowWinErrorW(comError.ErrorMessage(), L"COM error", line, file);

            DebugBreak();

            throw std::exception("COM error.");
        };

        return hResult;
    };
    #else

    // A macro used to debug COM functions.
    // Creates a scoped variable of type HRESULT and stores the retured function value,
    // using COM's FAILED macro, checks if the result is valid or not
    #define COMCALL(function) _COM_CALL(function, __LINE__, __FILE__)

    inline HRESULT _COM_CALL(HRESULT hResult,
                             std::intmax_t line, const char* file)
    {
        if (FAILED(hResult))
        {
            _com_error comError(hResult);

            ShowWinErrorA(comError.ErrorMessage(), "COM error", line, file);

            DebugBreak();

            throw std::exception("COM error.");
        };

        return hResult;
    };

    #endif

    #pragma endregion


};


namespace WindowsUtilities
{
    namespace EX
    {
        #define WINCALL(result) WinCall(result, __LINE__, __FILEW__)

        template<typename TType1, typename TType2>
        static std::size_t CompareTypeCodes()
        {
            return (typeid(TType1).hash_code() == typeid(TType2).hash_code());
        };

        template<typename TValue>
        static TValue WinCall(const TValue& value, const std::intmax_t line, const wchar_t* file)
        {
            if (CompareTypeCodes<TValue, BOOL>())
            {
                if (!value)
                {
                    ShowWinErrorW(GetLastErrorAsStringW(), L"WinError", line, file);

                    DebugBreak();
                };

                return value;
            };

            if (CompareTypeCodes<TValue, HANDLE>())
            {
                if (reinterpret_cast<const HANDLE>(value) == nullptr)
                {
                    ShowWinErrorW(GetLastErrorAsStringW(), L"WinError", line, file);

                    DebugBreak();
                };

                return value;
            };

            throw std::exception("Unsupported / Unimplemented Type.");
        };

    };

};

// Utility debugger helpers
namespace WindowsUtilities
{

    /// <summary>
    /// Utility debugger helpers
    /// </summary>
    namespace Debug
    {

        /// <summary>
        /// A nice simple way of only breaking if a debugger IS attached. 
        /// No idea why this isn't a standard Windows function
        /// </summary>
        static void DebugBreakIfAttached()
        {
            if (IsDebuggerPresent())
                DebugBreak();
        };


        /// <summary>
        /// Launches the Visual Studio Just-in-Time debugger.
        /// </summary>
        /// <param name="stopAfterAttaching"></param>
        static void LaunchVSJITDebugger(bool stopAfterAttaching = false)
        {
            // No need to launch debugger if one is already present
            if (IsDebuggerPresent())
            {
                OutputDebugStringA("A debugger was already present\n");
                throw std::exception("A debugger is already attached");
            };

            // Before getting the system directory string, get it's length
            const std::uint32_t length = GetSystemDirectoryW(nullptr, 0);

            // This might happen ?
            if (length == 0)
            {
                OutputDebugStringA("A call to GetSystemDirectoryW() failed unexpectedly, returned length == 0\n");
                throw std::exception("System directory not found. Reinstall OS");
            };


            // Create the string that will hold the path to the debugger
            std::wstring debuggerPath;
            debuggerPath.reserve(length);
            debuggerPath.resize(length);

            // Get the actuall system directory string
            GetSystemDirectoryW(&(*debuggerPath.begin()), length);

            // GetSystemDirectoryW adds a '\0' character at the end of the returned system directory string,
            // simply overriding the last character which is the null terminator with a backslash and appending the name of the debugger is fine
            *(debuggerPath.end() - 1) = L'\\';
            debuggerPath.append(L"vsjitdebugger.exe");

            // vsjit launch parameters.
            // -p means the process ID to attach to
            std::wstring debuggerLaunchArgs;
            debuggerLaunchArgs.append(L" -p ");
            debuggerLaunchArgs.append(std::to_wstring(GetCurrentProcessId()));

            // Copying the string to a non-const wchar_t* because sometimes, 
            // certain processes modify the commandline string
            wchar_t* debuggerLaunchArgsCopy = new wchar_t[debuggerLaunchArgs.size() + 1] { 0 };
            // std::wcsncpy(debuggerLaunchArgsCopy, debuggerLaunchArgs.c_str(), debuggerLaunchArgs.size());

            wcscpy_s(debuggerLaunchArgsCopy, debuggerLaunchArgs.size() + 1, debuggerLaunchArgs.c_str());

            STARTUPINFOW startupInfo { 0 };
            startupInfo.cb = sizeof(startupInfo);

            PROCESS_INFORMATION processInformation { 0 };

            // Launch vsjit
            if (CreateProcessW(debuggerPath.c_str(),
                debuggerLaunchArgsCopy,
                nullptr,
                nullptr,
                false,
                0,
                nullptr,
                nullptr,
                &startupInfo,
                &processInformation) == false)
            {
                CloseHandle(processInformation.hThread);
                CloseHandle(processInformation.hProcess);

                std::string error = "CreateProcessW() call failed with code ";
                error.append(std::to_string(GetLastError()));
                error.append(" \nError message \n");
                error.append(winutils::GetLastErrorAsStringA());
                error.append("\n");

                OutputDebugStringA(error.c_str());

                throw std::exception(error.c_str());
            };

            DWORD debuggerExitCode = 0;

            do
            {
                Sleep(100);

                // Check if the user has closed the "choose vsjit debugger" window
                GetExitCodeProcess(processInformation.hProcess, &debuggerExitCode);
            }
            // Wait for a debugger to attach, 
            while ((IsDebuggerPresent() == false) &&
                   // or closing of the vsjit debugger window
                   (debuggerExitCode == STILL_ACTIVE));


            CloseHandle(processInformation.hThread);
            CloseHandle(processInformation.hProcess);

            if (stopAfterAttaching == true)
            {
                DebugBreakIfAttached();
            };
        };

    };

    namespace dbg = Debug;

};


// Input related utilities
namespace WindowsUtilities
{

    namespace Input
    {

        static std::vector<INPUT> StringToInputListA(const std::string& text)
        {
            std::vector<INPUT> inputListOut;
            inputListOut.reserve(text.size());

            for (const char& character : text)
            {
                const std::int16_t keyScanResult = VkKeyScanW(character);
                const std::uint8_t virtualKeyCode = LOBYTE(keyScanResult);
                const std::uint8_t keyShiftState = HIBYTE(keyScanResult);

                const std::uint32_t scanCode = MapVirtualKeyW(virtualKeyCode, MAPVK_VK_TO_VSC);

                const bool shiftHeld = keyShiftState & 1;

                if (shiftHeld == true)
                {
                    INPUT shiftInput = { 0 };
                    shiftInput.type = INPUT_KEYBOARD;
                    shiftInput.ki.wVk = VK_SHIFT;

                    inputListOut.push_back(shiftInput);
                };

                INPUT input = { 0 };
                input.type = INPUT_KEYBOARD;
                input.ki.wScan = static_cast<std::uint16_t>(scanCode);
                input.ki.dwFlags = KEYEVENTF_SCANCODE;

                inputListOut.push_back(input);

                INPUT inputRelease = { 0 };
                inputRelease.type = INPUT_KEYBOARD;
                inputRelease.ki.wScan = static_cast<std::uint16_t>(scanCode);
                inputRelease.ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;

                inputListOut.push_back(inputRelease);

                if (shiftHeld == true)
                {
                    INPUT shiftRelease = { 0 };
                    shiftRelease.type = INPUT_KEYBOARD;
                    shiftRelease.ki.wVk = VK_SHIFT;
                    shiftRelease.ki.dwFlags = KEYEVENTF_KEYUP;

                    inputListOut.push_back(shiftRelease);
                };
            };

            return inputListOut;
        };


        static void ChangeKeyboardLocale(const HKL newLocale, const std::uint32_t thread = 0, const std::uint32_t flags = 0)
        {
            HKL currentLocale = GetKeyboardLayout(thread);

            do
            {
                if (ActivateKeyboardLayout(newLocale, flags) != currentLocale)
                    continue;

                currentLocale = GetKeyboardLayout(thread);
            }
            while (LOBYTE(currentLocale) != LOBYTE(newLocale));
        };



        template <class _Rep = long long, class _Period = std::milli>
        static void SendKeyPressAsKeycode(const std::vector<std::uint8_t>& keyCodes, const std::chrono::duration<_Rep, _Period>& timeoutBetweenKeys = std::chrono::milliseconds(1))
        {
            INPUT input = { 0 };

            for (const std::uint8_t& keycode : keyCodes)
            {
                input.type = INPUT_KEYBOARD;
                input.ki.wVk = keycode;
                input.ki.dwFlags = 0;

                SendInput(1, &input, sizeof(input));

                input.ki.wVk = keycode;
                input.ki.dwFlags = KEYEVENTF_KEYUP;

                SendInput(1, &input, sizeof(input));
            };

            std::this_thread::sleep_for(timeoutBetweenKeys);
        };


        static std::size_t SendKeyPress(const std::vector<INPUT>& keys)
        {
            return SendInput(static_cast<std::uint32_t>(keys.size()), const_cast<INPUT*>(keys.data()), sizeof(INPUT));
        };


        template <class _Rep = long long, class _Period = std::milli>
        static std::size_t SendKeyPressForceLanguage(const std::vector<INPUT>& keys, const HKL locale, const std::chrono::duration<_Rep, _Period>& timeout = std::chrono::milliseconds(1))
        {
            std::size_t totalSentInputs = 0;

            for (const INPUT& key : keys)
            {
                const std::int16_t capsKeyState = GetKeyState(VK_CAPITAL);
                const bool capsOn = LOBYTE(capsKeyState) & 1;

                if (capsOn == true)
                {
                    SendKeyPressAsKeycode({ VK_CAPITAL });
                };

                ChangeKeyboardLocale(locale);

                totalSentInputs += SendInput(1, const_cast<INPUT*>(&key), sizeof(INPUT));
            };

            std::this_thread::sleep_for(timeout);

            return totalSentInputs;
        };

    };

};

#pragma warning(pop)