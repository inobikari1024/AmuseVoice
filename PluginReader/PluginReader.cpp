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


// オーディオ系定数
static size_t const SAMPLING_RATE = 44100;
static size_t const BLOCK_SIZE = 1024;
static size_t const BUFFER_MULTIPLICITY = 4;


static std::mutex process_mutex;

static auto get_process_lock = [&]() -> std::unique_lock<std::mutex> {
	return std::unique_lock<std::mutex>(process_mutex);
};

static HostApplication host(SAMPLING_RATE, BLOCK_SIZE);
static std::vector<VstPlugin> vstPlugins;


/* プラグインの読み込み */
bool __stdcall LoadPlugin(const char* _vstPath)
{
	vstPlugins.push_back(VstPlugin (_vstPath, SAMPLING_RATE, BLOCK_SIZE, &host));

	if (!vstPlugins.back().IsSynth()) {
		MessageBox(NULL, "VSTプラグインではありません", "読み込みエラー", MB_OK | MB_ICONERROR);
		vstPlugins.pop_back();
		return false;
	}
	return true;
}


bool __stdcall OpenWaveOutDevice(int _samplingRate, int _channel, int _blockSize, int _bufferMultiplicity) 
{
	/* オーディオデバイスの準備 */
	WaveOutProcessor	wave_out_;

	// デバイスオープン
	bool const open_device = wave_out_.OpenDevice(
			SAMPLING_RATE,
			2,	// 2ch
			BLOCK_SIZE,				// バッファサイズ。再生が途切れる時はこの値を増やす。ただしレイテンシは大きくなる。
			BUFFER_MULTIPLICITY,	// バッファ多重度。再生が途切れる時はこの値を増やす。ただしレイテンシは大きくなる。

			// デバイスバッファに空きがあるときに呼ばれるコールバック関数。
			// このアプリケーションでは、一つのVstPluginに対して合成処理を行い、合成したオーディオデータをWaveOutProcessorの再生バッファへ書き込んでいる。
			[&](short* _data, size_t _deviceChannel, size_t _sample) {

				auto lock = get_process_lock();

				// VstPluginに追加したノートイベントを
				// 再生用データとして実際のプラグイン内部に渡す
				//vst.ProcessEvents();

				// _sample分の時間のオーディオデータ合成
				float** syntheized = vstPlugins[0]->ProcessAudio(_sample);

				size_t const channels_to_be_played =
					std::min<size_t>(_deviceChannel, vst.GetEffect()->numOutputs);

				// 合成したデータをオーディオデバイスのチャンネル数以内のデータ領域に書き出し。
				// デバイスのサンプルタイプを16bit整数で開いているので、
				// VST側の-1.0 .. 1.0のオーディオデータを-32768 .. 32767に変換している。
				// また、VST側で合成したデータはチャンネルごとに列が分かれているので、
				// Waveformオーディオデバイスに流す前にインターリーブする。
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
		MessageBox(NULL, "オーディオデバイスの初期化に失敗しました", "初期化エラー", MB_OK | MB_ICONERROR);
		return false;
	}
	return true;
}

