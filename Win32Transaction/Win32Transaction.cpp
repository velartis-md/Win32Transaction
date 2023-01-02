//Copyright (c) 2022 Bruno van Dooren
//Permission is hereby granted, free of charge, to any person obtaining a copy
//of this software and associated documentation files (the "Software"), to deal
//in the Software without restriction, including without limitation the rights
//to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//copies of the Software, and to permit persons to whom the Software is
//furnished to do so, subject to the following conditions:
//The above copyright notice and this permission notice shall be included in all
//copies or substantial portions of the Software.
//THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
//SOFTWARE.
// 

#include "Win32Transaction.h"

using namespace std;

int main()
{
    wcout << L"Testing Win32 Transaction" << std::endl;

    errno_t err = 0;
    TCHAR fullPath[L_tmpnam_s];
    _wtmpnam_s(fullPath, L_tmpnam_s);
    if (err) {
        wcout << L"Error occurred creating unique filename" << std::endl;
        return 1;
    }
    wcout << L"Test file path:  \"" << fullPath << L"\"" << endl;

    HANDLE transaction = CreateTransaction(
        NULL,
        NULL,
        0,
        0,
        0,
        0,
        NULL
    );
    BOOL transactionResult;
    if (transaction == INVALID_HANDLE_VALUE)
    {
        wcout << L"Failed to create Win32 Transaction" << endl;
        return GetLastError();
    }

    HKEY    registryRoot = HKEY_CURRENT_USER;
    LPCTSTR regKey       = TEXT("Win32Transaction");
    LPCTSTR valueName    = TEXT("ConfigFile");

    DWORD error = NO_ERROR;
    BOOL result = TRUE;

    HANDLE fileHandle = NULL;
    HKEY regKeyHandle = NULL;
    const auto MSG = TEXT("Hello transacted world!");
    
    wcout << L"Calling RegCreateKeyTransacted" << endl;
    error = RegCreateKeyTransacted(
        registryRoot,
        regKey,
        0,
        NULL,
        REG_OPTION_NON_VOLATILE,
        KEY_READ | KEY_WRITE,
        NULL,
        &regKeyHandle,
        NULL,
        transaction,
        NULL
    );
    if (error != ERROR_SUCCESS) {
        wcout << L"  ^ Failed with: " << error << endl;
        goto Cleanup;
    }

    wcout << L"Calling RegSetValueEx" << endl;
    error = RegSetValueEx(
        regKeyHandle,
        valueName,
        0,
        REG_SZ,
        (const BYTE*)fullPath,
        (DWORD)((_tcslen(fullPath) + 1) * sizeof(TCHAR))
    );
    if (error != ERROR_SUCCESS) {
        wcout << L"  ^ Failed with: " << error << endl;
        goto Cleanup;
    }

    wcout << L"Calling CreateFileTransacted" << endl;
    fileHandle = CreateFileTransacted(
        fullPath,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL,
        transaction,
        NULL,
        NULL
    );
    if (fileHandle == INVALID_HANDLE_VALUE) {
        error = GetLastError();
        wcout << L"  ^ Failed with: " << error << endl;
        goto Cleanup;
    }

    wcout << L"Calling WriteFile" << endl;
    result = WriteFile(
        fileHandle,
        (const void*)MSG,
        (DWORD)((_tcslen(MSG)) * sizeof(TCHAR)),
        NULL,
        NULL
    );
    if (!result) {
        error = GetLastError();
        wcout << L"  ^ Failed with: " << error << endl;
        goto Cleanup;
    }

Cleanup:
    if (error == NO_ERROR) {
        wcout << L"Committing transaction" << endl;
        transactionResult = CommitTransaction(transaction);
    }
    else {
        wcout << L"Rolling back transaction" << endl;
        transactionResult = RollbackTransaction(transaction);
    }
    if (!transactionResult) {
        wcout << L"  ^ Failed with: " << GetLastError() << endl;
    }

    if (regKeyHandle != NULL) {
        CloseHandle(regKeyHandle);
    }
    if (fileHandle != NULL) {
        CloseHandle(fileHandle);
    }
    CloseHandle(transaction);
}
