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

/* WAVEHDR�̃��b�p�N���X */
/* WAVEHDR���Ǘ����A�o�C�g�P�ʂŎw�肵�������̃o�b�t�@�����蓖�Ă�B */
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

/* Wave�I�[�f�B�I�f�o�C�X���I�[�v�����A */
/* �f�o�C�X�ւ̏����o�����s���N���X */
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

	// �f�o�C�X�̃o�b�t�@���󂢂Ă���ꍇ�ɒǉ��Ńf�[�^��v������
	// �R�[���o�b�N�֐�
	// �f�o�C�X�̃f�[�^�`���͊ȒP�̂��߁A16bit�����t�������Œ�
	callback_function_t			callback_;
	std::mutex					initialLockMutex_;

	// �f�o�C�X���J��
	// �J���f�o�C�X�̎w��́A����WAVE_MAPPER�Œ�B
	// �ȒP�̂��ߗ�O���S���Ȃǂ͂��܂�l������Ă��Ȃ��_�ɒ��ӁB
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

		// �f�o�C�X���I�[�v����������܂�_callback���Ă΂�Ȃ��悤�ɂ��邽�߂̃��b�N
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

		// WAVEHDR�g�p�ςݒʒm���󂯎������Ƃ���
		// CALLBACK_FUNCTION���w��B
		// ����Ƀf�o�C�X���I�[�v���ł���ƁA
		// waveOutWrite���Ăяo�������WaveOutProcessor::waveOutProc�ɒʒm������悤�ɂȂ�B
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

	// �f�o�C�X�����
	void CloseDevice() {
		terminated_.store(true);
		processThread_.join();
		waveOutReset(hWaveOut_);
		waveOutClose(hWaveOut_);

		// waveOutReset���Ăяo�������Ƃ�
		// WAVEHDR�̎g�p�ςݒʒm�����邱�Ƃ�����B
		// �܂�AwaveOutReset���Ăяo���������
		// ������WAVEHDR������ł��Ȃ�(�f�o�C�X�ɂ���Ďg�p����������Ȃ�����)
		// ���̂��߁A�m���ɂ��ׂĂ�WAVEHDR�̉�����m�F����B
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

	// �f�o�C�X�I�[�v�����Ɏw�肵���R�[���o�b�N�֐����Ăяo���āA
	// �f�o�C�X�ɏo�͂���I�[�f�B�I�f�[�^����������B
	void PrepareData(WAVEHDR* header)
	{
		callback_(reinterpret_cast<short*>(header->lpData), channel_, blockSize_);
	}

	// �Đ��p�f�[�^�̏�����
	// WAVEHDR�̓���ւ������X�ƍs�����[�J�[�X���b�h
	void ProcessThread()
	{
		{
			std::unique_lock<std::mutex> lock(initialLockMutex_);
		}

		for (; ; ) {
			if (terminated_.load()) { break; }

			// �g�p�ς�WAVEHDR�̊m�F
			for (auto& header : headers_) {
				DWORD_PTR status = NULL;

				EnterCriticalSection(&criticalSection_);
				status = header->get()->dwUser;
				LeaveCriticalSection(&criticalSection_);

				// �g�p�ς�WAVEHDR��Unprepare���āA���g�p�t���O�𗧂Ă�B
				if (status == WaveHeader::DONE) {
					waveOutUnprepareHeader(hWaveOut_, header->get(), sizeof(WAVEHDR));
					header->get()->dwUser = WaveHeader::UNUSED;
				}
			}

			// ���g�pWAVEHDR���m�F
			for (auto& header : headers_) {
				DWORD_PTR status = NULL;

				EnterCriticalSection(&criticalSection_);
				status = header->get()->dwUser;
				LeaveCriticalSection(&criticalSection_);

				if (status == WaveHeader::UNUSED) {
					// �Đ��p�f�[�^������
					PrepareData(header->get());
					header->get()->dwUser = WaveHeader::USING;
					// waveOutPrepareHeader���Ăяo���O�ɁAdwFlags�͕K��0�ɂ���B
					header->get()->dwFlags = 0;

					// WAVEHDR��Prepare
					waveOutPrepareHeader(hWaveOut_, header->get(), sizeof(WAVEHDR));
					// �f�o�C�X�֏����o��(�����悤�ɓo�^)
					waveOutWrite(hWaveOut_, header->get(), sizeof(WAVEHDR));
				}
			}

			Sleep(1);
		}
	}

	// �f�o�C�X����̒ʒm���󂯎��֐�
	static void CALLBACK waveOutProc(HWAVEOUT hwo, UINT msg, DWORD_PTR instance, DWORD_PTR p1, DWORD_PTR /*p2*/)
	{
		reinterpret_cast<WaveOutProcessor*>(instance)->WaveCallback(hwo, msg, reinterpret_cast<WAVEHDR*>(p1));
	}

	// �}���`���f�B�A�n�̃R�[���o�b�N�֐���
	// �g����֐��Ɍ��肪����ȂǗv����������
	// ���ɁA���̊֐�����waveOutUnprepareHeader�Ȃǂ��Ăяo����WAVEHDR��
	// ���Z�b�g�������s������͂ł��Ȃ��B�����Ȃ��΃V�X�e�����f�b�h���b�N����B
	void WaveCallback(HWAVEOUT /*hwo*/, UINT msg, WAVEHDR* header)
	{
		switch (msg) {
		case WOM_OPEN:
			break;

		case WOM_CLOSE:
			break;

		case WOM_DONE:
			EnterCriticalSection(&criticalSection_);
			// �g�p�ς݃t���O�𗧂Ă�̂�
			header->dwUser = WaveHeader::DONE;
			LeaveCriticalSection(&criticalSection_);
			break;
		}
	}
};
