#pragma once
#include <memory>
#include <utility>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <functional>
#include <cassert>

#include <windows.h>
#include <mmsystem.h>

#pragma comment(lib, "winmm.lib")

/* WAVEHDRのラッパクラス */
/* WAVEHDRを管理し、バイト単位で指定した長さのバッファを割り当てる。 */
struct WaveHeader {
	enum { UNUSED, USING, DONE };

	WaveHeader(size_t _byteLength)
	{
		header_.lpData = new char[_byteLength];
		header_.dwBufferLength = _byteLength;
		header_.dwBytesRecorded = 0;
		header_.dwUser = UNUSED;
		header_.dwLoops = 0;
		header_.dwFlags = 0;
		header_.lpNext = nullptr;
		header_.reserved = 0;
	}

	~WaveHeader()
	{
		delete[] header_.lpData;
	}

	WaveHeader(WaveHeader&& _rhs)
		: header_(_rhs.header_)
	{
		WAVEHDR empty = {};
		_rhs.header_ = empty;
	}

	WaveHeader& operator=(WaveHeader&& _rhs)
	{
		WaveHeader(std::move(_rhs)).swap(_rhs);
		return *this;
	}

	void swap(WaveHeader& _rhs)
	{
		WAVEHDR tmp = header_;
		header_ = _rhs.header_;
		_rhs.header_ = tmp;
	}

	WAVEHDR* get() { return &header_; }

private:
	WAVEHDR	header_;
};

/* Waveオーディオデバイスをオープンし、 */
/* デバイスへの書き出しを行うクラス */
struct WaveOutProcessor
{
	WaveOutProcessor()
		: hWaveOut_(NULL)
		, terminated_(false)
		, blockSize_(0)
		, channel_(0)
		, multiplicity_(0)
	{
		InitializeCriticalSection(&criticalSection_);
	}

	~WaveOutProcessor() {
		assert(!hWaveOut_);
		DeleteCriticalSection(&criticalSection_);
	}

	CRITICAL_SECTION criticalSection_;
	HWAVEOUT hWaveOut_;
	size_t	 blockSize_;
	size_t	 multiplicity_;
	size_t	 channel_;
	std::vector<std::unique_ptr<WaveHeader>>	headers_;
	std::thread			processThread_;
	std::atomic<bool>	terminated_;


	typedef std::function<void(short* _data, size_t _channel, size_t _sample)>
		callback_function_t;

	// デバイスのバッファが空いている場合に追加でデータを要求する
	// コールバック関数
	// デバイスのデータ形式は簡単のため、16bit符号付き整数固定
	callback_function_t			callback_;
	std::mutex					initialLockMutex_;

	// デバイスを開く
	// 開くデバイスの指定は、現在WAVE_MAPPER固定。
	// 簡単のため例外安全性などはあまり考慮されていない点に注意。
	bool OpenDevice(size_t _samplingRate, size_t _channel, size_t _blockSize, size_t _multiplicity, callback_function_t _callback)
	{
		assert(0 < _blockSize);
		assert(0 < _multiplicity);
		assert(0 < _channel && _channel <= 2);
		assert(_callback);
		assert(!processThread_.joinable());

		blockSize_ = _blockSize;
		channel_ = _channel;
		callback_ = _callback;
		multiplicity_ = _multiplicity;

		// デバイスがオープン完了するまで_callbackが呼ばれないようにするためのロック
		std::unique_lock<std::mutex> lock(initialLockMutex_);

		terminated_ = false;
		processThread_ = std::thread([this] { ProcessThread(); });

		WAVEFORMATEX wf;
		wf.wFormatTag = WAVE_FORMAT_PCM;
		wf.nChannels = 2;
		wf.wBitsPerSample = 16;
		wf.nBlockAlign = wf.nChannels * (wf.wBitsPerSample / 8);
		wf.nSamplesPerSec = _samplingRate;
		wf.nAvgBytesPerSec = wf.nBlockAlign * wf.nSamplesPerSec;
		wf.cbSize = sizeof(WAVEFORMATEX);

		headers_.resize(multiplicity_);
		for (auto& header : headers_) {
			header.reset(new WaveHeader(_blockSize * _channel * sizeof(short)));
		}

		// WAVEHDR使用済み通知を受け取る方式として
		// CALLBACK_FUNCTIONを指定。
		// 正常にデバイスがオープンできると、
		// waveOutWriteを呼び出した後でWaveOutProcessor::waveOutProcに通知が来るようになる。
		MMRESULT const result =
			waveOutOpen(
				&hWaveOut_,
				0,
				&wf,
				reinterpret_cast<DWORD>(&WaveOutProcessor::waveOutProc),
				reinterpret_cast<DWORD_PTR>(this),
				CALLBACK_FUNCTION
			);

		if (result != MMSYSERR_NOERROR) {
			terminated_ = true;
			processThread_.join();
			terminated_ = false;
			hWaveOut_ = NULL;

			return false;
		}

		return true;
	}

	// デバイスを閉じる
	void CloseDevice() {
		terminated_.store(true);
		processThread_.join();
		waveOutReset(hWaveOut_);
		waveOutClose(hWaveOut_);

		// waveOutResetを呼び出したあとで
		// WAVEHDRの使用済み通知が来ることがある。
		// つまり、waveOutResetを呼び出した直後に
		// すぐにWAVEHDRを解放できない(デバイスによって使用中かもしれないため)
		// そのため、確実にすべてのWAVEHDRの解放を確認する。
		for (; ; ) {
			int left = 0;
			for (auto& header : headers_) {
				if (header->get()->dwUser == WaveHeader::DONE) {
					waveOutUnprepareHeader(hWaveOut_, header->get(), sizeof(WAVEHDR));
					header->get()->dwUser = WaveHeader::UNUSED;
				}
				else if (header->get()->dwUser == WaveHeader::USING) {
					left++;
				}
			}

			if (!left) { break; }
			Sleep(10);
		}
		hWaveOut_ = NULL;
	}

	// デバイスオープン時に指定したコールバック関数を呼び出して、
	// デバイスに出力するオーディオデータを準備する。
	void PrepareData(WAVEHDR* header)
	{
		callback_(reinterpret_cast<short*>(header->lpData), channel_, blockSize_);
	}

	// 再生用データの準備と
	// WAVEHDRの入れ替えを延々と行うワーカースレッド
	void ProcessThread()
	{
		{
			std::unique_lock<std::mutex> lock(initialLockMutex_);
		}

		for (; ; ) {
			if (terminated_.load()) { break; }

			// 使用済みWAVEHDRの確認
			for (auto& header : headers_) {
				DWORD_PTR status = NULL;

				EnterCriticalSection(&criticalSection_);
				status = header->get()->dwUser;
				LeaveCriticalSection(&criticalSection_);

				// 使用済みWAVEHDRはUnprepareして、未使用フラグを立てる。
				if (status == WaveHeader::DONE) {
					waveOutUnprepareHeader(hWaveOut_, header->get(), sizeof(WAVEHDR));
					header->get()->dwUser = WaveHeader::UNUSED;
				}
			}

			// 未使用WAVEHDRを確認
			for (auto& header : headers_) {
				DWORD_PTR status = NULL;

				EnterCriticalSection(&criticalSection_);
				status = header->get()->dwUser;
				LeaveCriticalSection(&criticalSection_);

				if (status == WaveHeader::UNUSED) {
					// 再生用データを準備
					PrepareData(header->get());
					header->get()->dwUser = WaveHeader::USING;
					// waveOutPrepareHeaderを呼び出す前に、dwFlagsは必ず0にする。
					header->get()->dwFlags = 0;

					// WAVEHDRをPrepare
					waveOutPrepareHeader(hWaveOut_, header->get(), sizeof(WAVEHDR));
					// デバイスへ書き出し(されるように登録)
					waveOutWrite(hWaveOut_, header->get(), sizeof(WAVEHDR));
				}
			}

			Sleep(1);
		}
	}

	// デバイスからの通知を受け取る関数
	static void CALLBACK waveOutProc(HWAVEOUT hwo, UINT msg, DWORD_PTR instance, DWORD_PTR p1, DWORD_PTR /*p2*/)
	{
		reinterpret_cast<WaveOutProcessor*>(instance)->WaveCallback(hwo, msg, reinterpret_cast<WAVEHDR*>(p1));
	}

	// マルチメディア系のコールバック関数は
	// 使える関数に限りがあるなど要求が厳しい
	// 特に、この関数内でwaveOutUnprepareHeaderなどを呼び出してWAVEHDRの
	// リセット処理を行ったりはできない。さもなくばシステムがデッドロックする。
	void WaveCallback(HWAVEOUT /*hwo*/, UINT msg, WAVEHDR* header)
	{
		switch (msg) {
		case WOM_OPEN:
			break;

		case WOM_CLOSE:
			break;

		case WOM_DONE:
			EnterCriticalSection(&criticalSection_);
			// 使用済みフラグを立てるのみ
			header->dwUser = WaveHeader::DONE;
			LeaveCriticalSection(&criticalSection_);
			break;
		}
	}
};
