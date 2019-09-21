#pragma warning(push)
#pragma warning(disable: 4996)

#include <algorithm>
#include <tchar.h>

#include "Vst2Plugin.h"


HostApplication::HostApplication(size_t _samplingRate, size_t _blockSize)
		: samplingRate_(_samplingRate)
		, blockSize_(_blockSize)
	{}

	VstIntPtr VSTCALLBACK VstHostCallback(AEffect* effect, VstInt32 opcode, VstInt32 index, VstIntPtr value, void* ptr, float opt)
	{
		// VstPluginの初期化が完了するまではこっち
		if (!effect || !effect->user) {
			switch (opcode) {
			case audioMasterVersion:
				return kVstVersion;
			default:
				return 0;
			}
		}
		else {
			// VstPluginの初期化が完了すると、effect->userに、effectを保持しているVstPluginのアドレスが入っている
			VstPlugin* vst = static_cast<VstPlugin*>(effect->user);
			return vst->GetHost()._callback(vst, opcode, index, value, ptr, opt);
		}
	}

	VstIntPtr HostApplication::_callback(VstPlugin* vst, VstInt32 opcode, VstInt32 index, VstIntPtr value, void* ptr, float opt)
	{
		int result = false;
		opt; // 未使用の変数の警告を抑制

		switch (opcode)
		{
		case audioMasterAutomate:
			// Pluginからの操作の通知
			// オートメーション操作の記録に対応したVSTホストは
			// ここで渡ってきたデータをオートメーションエンベロープに記録する
			break;

		case audioMasterVersion:
			result = kVstVersion;
			break;

		case audioMasterCurrentId:
			// kPlugCategShellタイプのプラグインに対して、全てのサブプラグイン名を展開するときにのみ呼ばれる
			return vst->GetEffect()->uniqueID;

		case audioMasterIdle:
			// VSTプラグインのアイドル時間をVSTホスト渡す
			// VSTホストは、全ての開いているプラグインエディタに対して、effEditIdleを呼び出したりする
		{
			//if (vst->IsEditorOpened()) {
				//vst->dispatcher(effEditIdle, 0, 0, 0, 0);
			//}
		}
		break;

		//Deprecated
			//case audioMasterPinConnected:
				//break;
		//Deprecated
			//case audioMasterWantMidi:
				//break;

		case audioMasterGetTime:
			// VSTホストの現在の時刻情報を返す
			timeInfo_.samplePos = 0;
			timeInfo_.sampleRate = samplingRate_;
			timeInfo_.nanoSeconds = GetTickCount() * 1000.0 * 1000.0;
			timeInfo_.ppqPos = 0;
			timeInfo_.tempo = 120.0;
			timeInfo_.barStartPos = 0;
			timeInfo_.cycleStartPos = 0;
			timeInfo_.cycleEndPos = 0;
			timeInfo_.timeSigNumerator = 4;
			timeInfo_.timeSigDenominator = 4;
			timeInfo_.smpteOffset = 0;
			timeInfo_.smpteFrameRate = kVstSmpte24fps;
			timeInfo_.samplesToNextClock = 0;
			timeInfo_.flags = (kVstNanosValid | kVstPpqPosValid | kVstTempoValid | kVstTimeSigValid);

			return reinterpret_cast<VstIntPtr>(&timeInfo_);

		case audioMasterProcessEvents:
			// プラグインから送られてきたイベントを処理する
			// processReplacingの呼び出しの中から呼ばれる。
			break;

			//Deprecated
				//case audioMasterSetTime:
					//break;
			//Deprecated
				//case audioMasterTempoAt:
					//break;
			//Deprecated
				//case audiomasterGetNumAutomatableParameters:
					//break;
			//Deprecated
				//case audioMasterGetParameterQuantization:
					//break;

		case audioMasterIOChanged:
			break;

			//Deprecated
				//case audioMasterNeedIdle:
					//break;

		case audioMasterSizeWindow:
			// プラグインからのサイズ変更要請
		{
			//size_t const width = index;
			//size_t const height = value;
			//vst->SetWindowSize(width, height);
			return 0; //return 1 if supported to set window size
		}

		case audioMasterGetSampleRate:
			// サンプリングレート問い合わせ
			return samplingRate_;

		case audioMasterGetBlockSize:
			// ブロックサイズ問い合わせ
			return blockSize_;

		case audioMasterGetInputLatency:
			// 入力レイテンシ問い合わせ
			return 0;

		case audioMasterGetOutputLatency:
			// 出力レイテンシ問い合わせ
			return 0;

			//Deprecated
				//case audioMasterGetPreviousPlug:
					//break;
			//Deprecated
				//case audioMasterGetNextPlug:
				//break;
			//Deprecated
				//case AudioMasterWillReplaceOrAccumulate:
					//break;

		case audioMasterGetCurrentProcessLevel:
			return kVstProcessLevelUnknown;

		case audioMasterGetAutomationState:
			return kVstAutomationOff;

		case audioMasterOfflineStart:
			break;

		case audioMasterOfflineRead:
			break;

		case audioMasterOfflineWrite:
			break;

		case audioMasterOfflineGetCurrentPass:
			break;

		case audioMasterOfflineGetCurrentMetaPass:
			break;

			//Deprecated
				//case audiomasterSetOutputSampleRate:
					//break;
			//Deprecated
				//case audioMasterGetputSpeakerArrangement:
					//break;

		case audioMasterGetVendorString:
		{
			static char const vendor_string[] = "hotwatermorning";
			static size_t const vendor_string_len = sizeof(vendor_string) / sizeof(char) - 1;
			std::copy(vendor_string, vendor_string + vendor_string_len, static_cast<char*>(ptr));
		}
		return 1;

		case audioMasterGetProductString:
		{
			static char const product_string[] = "Vst Host Test";
			static size_t const product_string_len = sizeof(product_string) / sizeof(char) - 1;
			std::copy(product_string, product_string + product_string_len, static_cast<char*>(ptr));
		}
		return 1;

		case audioMasterGetVendorVersion:
		{
			return 1;
		}

		case audioMasterVendorSpecific:
			break;

			//Deprecated
				//case audioMasterSetIcon:
					//break;

		case audioMasterCanDo:
			// ホストがサポートしている機能をVSTプラグインに通知
		{
			char const* do_list[] = {
				{ "sendVstEvents" },
				{ "sendVstMidiEvents" },
				{ "sizeWindow" },
				{ "startStopProcess" },
				{ "sendVstMidiEventFlagIsRealtime" } };

			for (auto elem : do_list) {
				if (strcmp(elem, static_cast<char const*>(ptr)) == 0) {
					return 1;
				}
			}
			//don't know
			return 0;
		}

		case audioMasterGetLanguage:
		{
			return kVstLangJapanese;
		}

		//Deprecated
			//case audioMasterOpenWindow:
				//break;

		//Deprecated
			//case audioMasterCloseWindow:
				//break;

		case audioMasterGetDirectory:
			// プラグインのDLLを含んでいるディレクトリの問い合わせ
			return reinterpret_cast<VstIntPtr>(vst->GetDirectory());

		case audioMasterUpdateDisplay:
			break;

		case audioMasterBeginEdit:
			break;

		case audioMasterEndEdit:
			break;

		case audioMasterOpenFileSelector:
			break;

		case audioMasterCloseFileSelector:
			break;

			//Deprecated
				//case audioMasterEditFile:
					//break;
			//Deprecated
				//case audioMasterGetChunkFile:
					//break;
			//Deprecated
				//case audioMasterGetInputSpeakerArrangement:
					//break;

		default:
			//unsupported opcodes
			;
		}
		return result;
	}

#pragma warning(pop)
