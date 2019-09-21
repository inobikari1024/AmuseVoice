#include <array>
#include <memory>

#include <atomic>
#include <optional>
#include <thread>
#include <condition_variable>

#include <windows.h>
#include <tchar.h>
#include <mmsystem.h>

#pragma warning(push)
#pragma warning(disable: 4996)
#include "C:\VST_SDK\vstsdk2.4\pluginterfaces\vst2.x\aeffectx.h"
#pragma warning(pop)

#include "Vst2Plugin.h"
#include "PluginReader.h"
#include "WaveOut.h"


// �I�[�f�B�I�n�萔
static size_t const SAMPLING_RATE = 44100;
static size_t const BLOCK_SIZE = 1024;
static size_t const BUFFER_MULTIPLICITY = 4;


static std::mutex process_mutex;

static auto get_process_lock = [&]() -> std::unique_lock<std::mutex> {
	return std::unique_lock<std::mutex>(process_mutex);
};

static HostApplication host(SAMPLING_RATE, BLOCK_SIZE);
static std::vector<VstPlugin> vstPlugins;


/* �v���O�C���̓ǂݍ��� */
bool __stdcall LoadPlugin(const char* _vstPath)
{
	vstPlugins.push_back(VstPlugin (_vstPath, SAMPLING_RATE, BLOCK_SIZE, &host));

	if (!vstPlugins.back().IsSynth()) {
		MessageBox(NULL, "VST�v���O�C���ł͂���܂���", "�ǂݍ��݃G���[", MB_OK | MB_ICONERROR);
		vstPlugins.pop_back();
		return false;
	}
	return true;
}


bool __stdcall OpenWaveOutDevice(int _samplingRate, int _channel, int _blockSize, int _bufferMultiplicity) 
{
	/* �I�[�f�B�I�f�o�C�X�̏��� */
	WaveOutProcessor	wave_out_;

	// �f�o�C�X�I�[�v��
	bool const open_device = wave_out_.OpenDevice(
			SAMPLING_RATE,
			2,	// 2ch
			BLOCK_SIZE,				// �o�b�t�@�T�C�Y�B�Đ����r�؂�鎞�͂��̒l�𑝂₷�B���������C�e���V�͑傫���Ȃ�B
			BUFFER_MULTIPLICITY,	// �o�b�t�@���d�x�B�Đ����r�؂�鎞�͂��̒l�𑝂₷�B���������C�e���V�͑傫���Ȃ�B

			// �f�o�C�X�o�b�t�@�ɋ󂫂�����Ƃ��ɌĂ΂��R�[���o�b�N�֐��B
			// ���̃A�v���P�[�V�����ł́A���VstPlugin�ɑ΂��č����������s���A���������I�[�f�B�I�f�[�^��WaveOutProcessor�̍Đ��o�b�t�@�֏�������ł���B
			[&](short* _data, size_t _deviceChannel, size_t _sample) {

				auto lock = get_process_lock();

				// VstPlugin�ɒǉ������m�[�g�C�x���g��
				// �Đ��p�f�[�^�Ƃ��Ď��ۂ̃v���O�C�������ɓn��
				//vst.ProcessEvents();

				// _sample���̎��Ԃ̃I�[�f�B�I�f�[�^����
				float** syntheized = vstPlugins[0]->ProcessAudio(_sample);

				size_t const channels_to_be_played =
					std::min<size_t>(_deviceChannel, vst.GetEffect()->numOutputs);

				// ���������f�[�^���I�[�f�B�I�f�o�C�X�̃`�����l�����ȓ��̃f�[�^�̈�ɏ����o���B
				// �f�o�C�X�̃T���v���^�C�v��16bit�����ŊJ���Ă���̂ŁA
				// VST����-1.0 .. 1.0�̃I�[�f�B�I�f�[�^��-32768 .. 32767�ɕϊ����Ă���B
				// �܂��AVST���ō��������f�[�^�̓`�����l�����Ƃɗ񂪕�����Ă���̂ŁA
				// Waveform�I�[�f�B�I�f�o�C�X�ɗ����O�ɃC���^�[���[�u����B
				for (size_t ch = 0; ch < channels_to_be_played; ++ch) {
					for (size_t fr = 0; fr < _sample; ++fr) {
						double const _sample = syntheized[ch][fr] * 32768.0;
						_data[fr * _deviceChannel + ch] =
							static_cast<short>(
								std::max<double>(-32768.0, std::min<double>(_sample, 32767.0))
								);
					}
				}
			}
	);

	if (open_device == false) {
		MessageBox(NULL, "�I�[�f�B�I�f�o�C�X�̏������Ɏ��s���܂���", "�������G���[", MB_OK | MB_ICONERROR);
		return false;
	}
	return true;
}

