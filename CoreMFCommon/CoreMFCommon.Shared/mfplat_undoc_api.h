#pragma once

#include <Windows.h>
#include <mfobjects.h>

typedef HRESULT (WINAPI* __MFCreateFileFromHandle)(MF_FILE_ACCESSMODE AccessMode,
	MF_FILE_OPENMODE OpenMode,
	MF_FILE_FLAGS fFlags,
	LPCWSTR pwszFileURL,
	HANDLE hFile,
	IMFByteStream **ppIByteStream); //win8
//same MFCreateFile.
//hFile can not is INVALID_HANDLE_VALUE.
//hFile must contain FILE_FLAG_OVERLAPPED.
//pwszFileURL not be NULL, can be L"";
//ͬMFCreateFile��
//hFile����������INVALID_HANDLE_VALUE��������CreateFileʱӦָ�������첽IO��FILE_FLAG_OVERLAPPED���ġ�
//pwszFileURL���ܸ�NULL�������һ���ַ���ָ�룬���⼴�ɡ�
//��Ҫ��hFile����CloseHandle����IMFByteStream�ӿ�Release�󣬻��Զ�CloseHandle��

typedef HRESULT (WINAPI* __MFGetRandomNumber)(BYTE* pbBuffer,DWORD dwLen);
//same CryptGenRandom.
//ȥ��CryptGenRandom��2���������ǡ�

typedef HRESULT (WINAPI* __MFCreateMemoryStream)(BYTE* pBuffer,UINT nBufferSize,DWORD dwReserved,IStream** ppstm);
//DO NOT COPY NEW pBuffer DUPLICATE!
//ppstm->Release ago, You can not FREE pBuffer.
//dwReserved must be a 0.
//combining MFSerialize***ToStream and MFDeserialize***FromStream to use.
//ע�⣬ppstm�ӿ�ֻ�Ǽ򵥵İ�װһ��pBufferָ�룬����ppstm->Read����ֱ�Ӳ���pBuffer�ġ�
//������ppstm->Releaseǰ�����ܶ�pBuffer����free����Ȼ�����Read��ᷢ������
//����dwReserved������0�������ʵ������dwFlags��
//�˺���һ�����MFSerialize***ToStream��MFDeserialize***FromStreamʹ�á�

typedef HRESULT (WINAPI* __MFAppendCollection)(IMFCollection* pAppendTo,IMFCollection* pAppend);
typedef HRESULT (WINAPI* __MFEnumLocalMFTRegistrations)(IUnknown** ppunk); //win8
typedef HRESULT (WINAPI* __MFClearLocalMFTs)(); //win8
typedef DWORD (WINAPI* __MFGetPlatformVersion)(); //win8
typedef DWORD (WINAPI* __MFGetPlatformFlags)(); //win8