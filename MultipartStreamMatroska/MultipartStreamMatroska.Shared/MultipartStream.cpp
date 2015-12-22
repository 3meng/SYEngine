#include "MultipartStream.h"

static const DWORD kPropertyStoreId[] = {
	MFNETSOURCE_BYTESRECEIVED_ID,
	MFNETSOURCE_PROTOCOL_ID,
	MFNETSOURCE_TRANSPORT_ID,
	MFNETSOURCE_CACHE_STATE_ID,
	MFNETSOURCE_DOWNLOADPROGRESS_ID};

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
static bool CheckWin10MatroskaSource(IMFAsyncCallback* cb)
{
	if (cb == NULL)
		return false;
	void* p = *(void**)cb;
	MEMORY_BASIC_INFORMATION mbi = {};
	VirtualQuery(p, &mbi, sizeof(mbi));
	HMODULE hmod = (HMODULE)mbi.AllocationBase;
	WCHAR buf[MAX_PATH] = {};
	GetModuleFileNameW(hmod, buf, MAX_PATH);
	wcslwr(buf);
	return wcsstr(buf, L"mfmkvsrcsnk.dll") != NULL;
}
#endif

HRESULT MultipartStream::QueryInterface(REFIID iid, void** ppv)
{
	if (ppv == NULL)
		return E_POINTER;
	*ppv = NULL;

	if (iid == IID_IUnknown ||
		iid == IID_IMFByteStream)
		*ppv = static_cast<IMFByteStream*>(this);
	else if (iid == IID_IMFByteStreamTimeSeek)
		*ppv = static_cast<IMFByteStreamTimeSeek*>(this);
	else if (iid == IID_IMFByteStreamBuffering)
		*ppv = static_cast<IMFByteStreamBuffering*>(this);
	else if (iid == IID_IMFByteStreamCacheControl)
		*ppv = static_cast<IMFByteStreamCacheControl*>(this);
	else if (iid == IID_IMFAttributes)
		*ppv = static_cast<IMFAttributes*>(this);
	else if (iid == IID_IMFGetService)
		*ppv = static_cast<IMFGetService*>(this);
	else if (iid == IID_IPropertyStore)
		*ppv = static_cast<IPropertyStore*>(this);
	else
		return E_NOINTERFACE;

	AddRef();
	return S_OK;
}

HRESULT MultipartStream::GetCount(DWORD* cProps)
{
	if (cProps == NULL)
		return E_POINTER;
	*cProps = _countof(kPropertyStoreId);
	return S_OK;
}

HRESULT MultipartStream::GetAt(DWORD iProp, PROPERTYKEY *pkey)
{
	if (pkey == NULL)
		return E_POINTER;
	if (iProp >= _countof(kPropertyStoreId))
		return E_INVALIDARG;

	pkey->fmtid = MFNETSOURCE_STATISTICS;
	pkey->pid = kPropertyStoreId[iProp];
	return S_OK;
}

HRESULT MultipartStream::GetValue(REFPROPERTYKEY key,PROPVARIANT *pv)
{
	if (pv == NULL)
		return E_POINTER;
	if (key.fmtid != MFNETSOURCE_STATISTICS)
		return MF_E_NOT_FOUND;

	if (key.pid != MFNETSOURCE_BYTESRECEIVED_ID &&
		key.pid != MFNETSOURCE_PROTOCOL_ID &&
		key.pid != MFNETSOURCE_TRANSPORT_ID &&
		key.pid != MFNETSOURCE_CACHE_STATE_ID &&
		key.pid != MFNETSOURCE_DOWNLOADPROGRESS_ID)
		return MF_E_NOT_FOUND;

	PropVariantInit(pv);
	pv->vt = VT_I4;
	switch (key.pid)
	{
	case MFNETSOURCE_BYTESRECEIVED_ID:
		pv->vt = VT_UI8;
		pv->uhVal.QuadPart = _stm_length;
		break;
	case MFNETSOURCE_PROTOCOL_ID:
		if (MFGetAttributeUINT32(_attrs.Get(), MF_BYTESTREAM_TRANSCODED, 0) == 1234)
			pv->lVal = 1; //MFNETSOURCE_HTTP
		else
			pv->lVal = 3; //MFNETSOURCE_FILE
		break;
	case MFNETSOURCE_TRANSPORT_ID:
		pv->lVal = 1; //1 = MFNETSOURCE_TCP, 0 = MFNETSOURCE_UDP
		break;
	case MFNETSOURCE_CACHE_STATE_ID:
		pv->lVal = MFNETSOURCE_CACHE_STATE::MFNETSOURCE_CACHE_UNAVAILABLE;
		break;
	case MFNETSOURCE_DOWNLOADPROGRESS_ID:
		pv->lVal = _download_progress;
		break;
	}
	return S_OK;
}

HRESULT MultipartStream::GetService(REFGUID guidService, REFIID riid, LPVOID *ppvObject)
{
	if (guidService == MFNETSOURCE_STATISTICS_SERVICE)
		return QueryInterface(riid,ppvObject);
	return MF_E_UNSUPPORTED_SERVICE;
}

HRESULT MultipartStream::GetCapabilities(DWORD *pdwCapabilities)
{
	if (pdwCapabilities == NULL)
		return E_POINTER;
	*pdwCapabilities = MFBYTESTREAM_IS_READABLE|MFBYTESTREAM_IS_SEEKABLE|
		MFBYTESTREAM_IS_REMOTE|(_download_progress > 99 ? 0:MFBYTESTREAM_IS_PARTIALLY_DOWNLOADED);
	//���Զ���Seek��������Զ�̷�������
	return S_OK;
}

HRESULT MultipartStream::GetLength(QWORD *pqwLength)
{
	if (pqwLength == NULL)
		return E_POINTER;
	std::lock_guard<decltype(_mutex)> lock(_mutex);
	*pqwLength = _stm_length;
	return S_OK;
}

HRESULT MultipartStream::GetCurrentPosition(QWORD *pqwPosition)
{
	if (pqwPosition == NULL)
		return E_POINTER;
	std::lock_guard<decltype(_mutex)> lock(_mutex);
	*pqwPosition = _stm_cur_pos;
	return S_OK;
}

HRESULT MultipartStream::IsEndOfStream(BOOL *pfEndOfStream)
{
	if (pfEndOfStream == NULL)
		return E_POINTER;
	*pfEndOfStream = _flag_eof ? TRUE:FALSE;
	return S_OK;
}

HRESULT MultipartStream::Read(BYTE *pb, ULONG cb, ULONG *pcbRead)
{
	//��2�δ���
	//1�����Read��ʼʱ�ĵ�ǰλ������Header+��һ��Cluster(Frame)�Ĵ�С�ڣ����ṩheader�����ݡ�
	//2�������������Ĵ�С���ṩMatroskaReadFile�����ݡ�
	if (pb == NULL || cb == 0)
		return S_OK;
	if (pcbRead != NULL)
		*pcbRead = 0;
	unsigned size = ReadFile(pb, cb); //ͬ����
	if (pcbRead)
		*pcbRead = size;
	return S_OK;
}

HRESULT MultipartStream::BeginRead(BYTE *pb, ULONG cb, IMFAsyncCallback *pCallback, IUnknown *punkState)
{
	if (pb == NULL ||
		cb == 0 ||
		pCallback == NULL)
		return E_INVALIDARG;

	std::lock_guard<decltype(_mutex)> lock(_mutex);
	if (!ThreadIsRun()) {
		if (!ThreadStart()) //�����첽���߳�
			return E_ABORT;
	}

	_async_read_ps.pb = pb;
	_async_read_ps.size = cb;
	_async_read_ps.callback = pCallback; //�����ȡ��ɺ����Ӧcb
	_async_read_ps.punkState = punkState;
	_async_read_ps.AddRef();
	SetEvent(_event_async_read); //֪ͨ�첽�߳���������
	return S_OK;
}

HRESULT MultipartStream::EndRead(IMFAsyncResult *pResult, ULONG *pcbRead)
{
	std::lock_guard<decltype(_mutex)> lock(_mutex);
	if (_async_read_ps.callback == NULL)
		return E_UNEXPECTED;
	if (pResult == NULL)
		return E_INVALIDARG;

	if (pcbRead)
		*pcbRead = _async_read_ps.cbRead; //���ض�ȡ�����ֽ���
	_async_read_ps.Release();
	memset(&_async_read_ps, 0, sizeof(_async_read_ps));
	return S_OK;
}

HRESULT MultipartStream::Seek(MFBYTESTREAM_SEEK_ORIGIN SeekOrigin, LONGLONG llSeekOffset, DWORD dwSeekFlags, QWORD *pqwCurrentPosition)
{
	/*
	Seek��ʱ���кܶ��������������֪��������Header��Ϣ��MemoryStream������MKV���ļ�ͷ+��һ��Cluster(Frame)���ֽ������ݡ�
	Ϊʲô��Ҫ��һ��Cluster�����ݣ���ΪDemuxer���һ�㶼��Parse��MKVͷ�ĵ�һ��Cluster�ĵط�ֹͣ�ģ����û�ҵ���һ��Cluster��Demux���ܻ�ʧ�ܡ�
	1���ոմ򿪣�Open���к�״̬ΪAfterOpen������û���������е�MatroskaReadFile��ʱ�� -> ����һ��Header��MemoryStreamָ�뵽ָ����λ�þ��У�һ�������Demux������ļ�ͷ��
	2������Ѿ�����MatroskaReadFile���̣�stm_cur_pos����Header��Stream��С����Seek��ָ���ֲ�����Header��Stream���ȣ�����Reset������ȥSeek��Header��Stream����Ҫ������һ��Packet��Ȼ����״̬ΪAfterOpen����Seek���ֽ�ָ�����Header��Stream������ʧ�ܣ��û�Ӧ����TimeSeek��
	3�������״̬ΪAfterSeek���������Seek��������ô�����������Ѿ�������MatroskaReadFile���̣����ڶ�������
	4�����TimeSeek��state���AfterSeek������Ҫ������һ��Packet��Ҳ����Ҫ��Header��Stream��ʼ��ֱ�ӽ���MatroskaReadFile���̡�
	5�����TimeSeek��ʱ����0.0����ͬ�ڶ���������ת����Reset��
	*/

	std::lock_guard<decltype(_mutex)> lock(_mutex);
	if (SeekOrigin == 0x10000)
	{
		//FFmpeg avio_size...
		*pqwCurrentPosition = _stm_length;
		return S_OK;
	}
	if ((dwSeekFlags & MFBYTESTREAM_SEEK_FLAG_CANCEL_PENDING_IO) != 0) {
		if (ThreadIsRun()) {
			//���Ҫ��Seekʱȡ���ȴ��е�IO
			//����һ�������ȴ����ȴ��첽���̷߳���
			SetEvent(_event_exit_thr);
			ThreadWait(); //�ȴ��첽��ȡ�߳��˳�
			ResetEvent(_event_exit_thr);
		}
	}

	//��ǰ�Ƿ��Ѿ�����ReadFile����
	bool enter_matroska_read_file = _stm_cur_pos > _header.GetLength();
	if (_state == AfterSeek) //�����TimeSeek�󣬿϶����Ѿ�����ReadFile
		enter_matroska_read_file = true;

	if (llSeekOffset > INT32_MAX)
		return E_FAIL;

	unsigned offset = (unsigned)llSeekOffset;
	if (enter_matroska_read_file) {
		Reset(false); //����
		_stm_cur_pos = 0;
		_state = AfterOpen; //���Openʱ��״̬
		if (SeekOrigin == msoCurrent)
			return S_OK;
		if (offset > _header.GetLength())
			return S_OK;
		_header.SetOffset(offset);
	}else{
		if (SeekOrigin == msoCurrent)
			offset += _header.GetOffset();
		if (offset > _header.GetLength())
			return E_FAIL;
		_header.SetOffset(offset); //ֱ��seekͷ����
	}
	_stm_cur_pos = _header.GetOffset();
	if (pqwCurrentPosition)
		*pqwCurrentPosition = _stm_cur_pos;
	_flag_eof = false;
	return S_OK;
}

HRESULT MultipartStream::Close()
{
	std::lock_guard<decltype(_mutex)> lock(_mutex);
	if (_closed)
		return S_OK;

	SetEvent(_event_exit_thr); //֪ͨ�첽���߳̿����˳���
	ThreadWait(); //ͬ���ȴ��첽���߳��˳���������ڣ�

	if (_async_time_seek.seekThread) {
		delete _async_time_seek.seekThread;
		_async_time_seek.seekThread = NULL;
	}

	DeInit(); //���ٸ�����Դ
	_header.SetLength(0);
	_header.Buffer()->Free();
	_closed = true; //�رձ��
	return S_OK;
}

HRESULT MultipartStream::IsTimeSeekSupported(BOOL *pfTimeSeekIsSupported)
{
	if (pfTimeSeekIsSupported == NULL)
		return E_POINTER;
	*pfTimeSeekIsSupported = TRUE; //mediasourceͨ������ж�stream�ǲ���֧��timeseek
	return S_OK;
}

HRESULT MultipartStream::TimeSeek(QWORD qwTimePosition)
{
	std::lock_guard<decltype(_mutex)> lock(_mutex);
	if (MFGetAttributeUINT32(_attrs.Get(), MF_BYTESTREAM_TRANSCODED, 0) == 0)
		return SeekTo((double)qwTimePosition / 10000000.0) ? S_OK : E_FAIL;

	if (_async_time_seek.seekThread)
		WaitTimeSeekResult();

	_async_time_seek.result = false;
	double time = (double)qwTimePosition / 10000000.0;
	auto thr = new(std::nothrow) std::thread(&MultipartStream::DoTimeSeekAsync,this,time);
	if (thr == NULL)
		return E_OUTOFMEMORY;

	_async_time_seek.seekThread = thr;
	WaitForSingleObjectEx(_async_time_seek.eventStarted, INFINITE, FALSE);
	return S_OK;
}

HRESULT MultipartStream::EnableBuffering(BOOL fEnable)
{
	if (fEnable == FALSE)
		return StopBackgroundTransfer();

#ifdef _DEBUG
	OutputDebugStringA("MultipartStream::EnableBuffering.");
#endif
	return S_OK;
}

HRESULT MultipartStream::StopBackgroundTransfer()
{
	std::lock_guard<decltype(_mutex)> lock(_mutex);
	if (GetDownloadManager()) {
		for (unsigned i = 0; i < GetItemCount(); i++)
			DownloadItemPause(i); //��ͣ���к�̨��������
	}
	_flag_user_stop = true;
	return S_OK;
}

bool MultipartStream::Open()
{
	std::lock_guard<decltype(_mutex)> lock(_mutex);

	if (!Init(_list_file)) //��ʼ��������Դ�����Ĵ���
		return false;
	if (MatroskaHeadBytes()->Length() == 0)
		return false;

	_header_prue_size = MatroskaHeadBytes()->Length(); //MKVͷ����һ��Clusterǰ��
	_header.SetOffset(0);
	_header.Write(MatroskaHeadBytes()->Get<void*>(), MatroskaHeadBytes()->Length());

	//ȡ�õ�һ��Packet��Cluster��������
	unsigned first_size = MatroskaInternalBuffer()->GetLength();
	MemoryBuffer buf;
	if (!buf.Alloc(first_size))
		return false;

	//��MKVͷ����Append��һ��Cluster������
	if (!MatroskaReadFile(buf.Get<void*>(), first_size))
		return false;
	_header.Write(buf.Get<void*>(), first_size);
	_header.SetOffset(0);

	_stm_length = InternalGetStreamSize(); //������Ƭ�Ĵ�С��������Ԥ���ģ�
	if (_stm_length == 0)
		return false;
	if (_stm_length == GetItems()->Size)
		_stm_length *= GetItemCount();

	_attrs->SetUINT64(MF_BYTESTREAM_DURATION, GetConfigs()->DurationMs * 10000);
	_attrs->SetString(MF_BYTESTREAM_ORIGIN_NAME, _list_file);
	_attrs->SetString(MF_BYTESTREAM_CONTENT_TYPE, L"video/x-matroska");
	if (GetItems()->Url[1] != L':') { //���Լ���MFSource��һ��flag
		_attrs->SetString(MF_BYTESTREAM_CONTENT_TYPE, L"video/force-network-stream");
		_attrs->SetUINT32(MF_BYTESTREAM_TRANSCODED, 1234);
	}

	_state = AfterOpen; //״̬��Open��
	return true;
}

void MultipartStream::ThreadInvoke(void*)
{
	AddRef();
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
#endif
	HANDLE handles[2] = {_event_async_read, _event_exit_thr};
	while (1)
	{
		DWORD state = WaitForMultipleObjectsEx(2, handles, FALSE, INFINITE, FALSE);
		if (state != 0) //�������_event_async_read�����Ǿ���exit���ߴ����˳��߳�
			break;

		ComPtr<IMFAsyncResult> result;
		{
			std::lock_guard<decltype(_mutex)> lock(_mutex);
			MFCreateAsyncResult(NULL,
				_async_read_ps.callback,
				_async_read_ps.punkState,
				&result);
			result->SetStatus(S_OK);
			//����ȡ
			_async_read_ps.cbRead = ReadFile(_async_read_ps.pb, _async_read_ps.size);
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
			if (CheckWin10MatroskaSource(_async_read_ps.callback)) {
				if (_async_read_ps.cbRead > 0)
					_async_read_ps.cbRead += ReadFile(_async_read_ps.pb + _async_read_ps.cbRead,
						_async_read_ps.size - _async_read_ps.cbRead);
			}
#endif
		}
		MFInvokeCallback(result.Get()); //֪ͨ�첽callback����EndRead
	}
}

unsigned MultipartStream::InternalGetStreamSize()
{
	if (GetItemCount() == 0)
		return false;
	unsigned size = 0;
	for (unsigned i = 0; i < GetItemCount(); i++)
		size += (GetItems() + i)->Size;
	return size;
}

double MultipartStream::InternalGetStreamDuration()
{
	if (GetItemCount() == 0)
		return false;
	double time = 0;
	for (unsigned i = 0; i < GetItemCount(); i++)
		time += (GetItems() + i)->Duration;
	return time;
}

unsigned MultipartStream::ReadFile(PBYTE pb, unsigned size)
{
	std::lock_guard<decltype(_mutex)> lock(_mutex);
	if (_async_time_seek.seekThread) {
		if (!WaitTimeSeekResult())
			return 0; //TimeSeek async failed.
	}

	unsigned allow_size = size;
	if (allow_size == 0)
		return 0;

	if (_stm_cur_pos < _header.GetLength() && _state == AfterOpen)
	{
		//��ǰָ����MKVͷ+��һ��Cluster�ķ�Χ�ڣ����Ҳ���TimeSeek��
		if (allow_size > _header.GetPendingLength())
			allow_size = _header.GetPendingLength();
		_header.Read(pb, allow_size); //��ֱ�Ӷ������Χ�ڵ�����
		_stm_cur_pos = _header.GetOffset();
	}else{
		//��ǰָ���Ѿ��������Ϸ�Χ��������AfterSeek...
		if (_flag_user_stop) {
			_flag_user_stop = false;
			DownloadItemStart(GetIndex()); //����û���ͣ�����أ��ָ�
		}
		//����ȡ�����ĵ���
		allow_size = MatroskaReadFile(pb, size);
		_stm_cur_pos += allow_size;

		//�����ǰ�����ʱ���Ѿ��ﵽ��Ҫ������һ����ʱ��
		if (int(GetCurrentTotalTime() - GetCurrentTimestamp()) <= 
			GetConfigs()->PreloadNextPartRemainSeconds)
			DownloadNextItem(); //������һ����item������

		/*
		//�����ǰ��������part�Ѿ����룬������һ��
		if (GetCurrentPartReadSize() > 
			(GetCurrentPartTotalSize() / 2))
			DownloadNextItem(); //������һ����item������
		*/
	}
	if (allow_size == 0)
		_flag_eof = true;
	return allow_size;
}

bool MultipartStream::SeekTo(double time)
{
	if (time < 0.1) //��С��ֱ��Reset
		return Reset(true);

	if (!MatroskaTimeSeek(time)) //��TimeSeek
		return false;
	_flag_eof = false;
	_state = AfterSeek; //״̬��AfterOpen���AfterSeek����TimeSeek��
	return true;
}

bool MultipartStream::Reset(bool timeseek)
{
	if (timeseek) {
		//AfterSeek
		//����Ҫ������һ��packet
		if (!MatroskaTimeSeek(0.0))
			return Reset(false);
	}else{
		//AfterOpen
		//��Ҫ������һ��packet
		if (!ReInit(false))
			return false;
		MatroskaReadFile(NULL, 0, 1); //skip first pkt.
		return true;
	}
	for (unsigned i = 1; i < GetItemCount(); i++)
		DownloadItemStop(i);
	return true;
}

bool MultipartStream::WaitTimeSeekResult()
{
	_async_time_seek.seekThread->join();
	delete _async_time_seek.seekThread;
	_async_time_seek.seekThread = NULL;
	return _async_time_seek.result;
}

void MultipartStream::DoTimeSeekAsync(double time)
{
	AddRef();
	SetEvent(_async_time_seek.eventStarted);
	_async_time_seek.result = SeekTo(time);
	Release();
}