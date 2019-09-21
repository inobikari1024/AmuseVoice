#pragma once
#include <cstdlib>
#include <vector>
#include <string>
#include <stdexcept>
#include <array>
#include <mutex>
#include <cassert>

#pragma warning(push)
#pragma warning(disable: 4996)
#include "C:\VST_SDK\vstsdk2.4\pluginterfaces\vst2.x\aeffectx.h"
#pragma warning(pop)

#include <Windows.h>
#include <mmsystem.h>

#pragma comment(lib,"winmm.lib")

#include "Module.h"


struct VstPlugin;

struct HostApplication
{
	HostApplication(size_t _samplingRate, size_t _blockSize);

	VstIntPtr _callback(
		VstPlugin* _vst,
		VstInt32 _opcode, VstInt32 _index, VstIntPtr _value,
		void* _ptr, float _opt
	);

private:
	size_t samplingRate_;
	size_t blockSize_;
	VstTimeInfo	timeInfo_;
};

// プラグイン内部からの要求などを受けて呼び出される
// Host側のコールバック関数
VstIntPtr VSTCALLBACK VstHostCallback(AEffect* _effect, VstInt32 _opcode, VstInt32 _index, VstIntPtr _value, void* _ptr, float _opt);


// VSTプラグインを表すクラス
struct VstPlugin
{
	// VSTプラグインのエントリポイントを表す関数の型
	// audioMasterCallbackというホスト側のコールバック関数を
	// 渡してこの関数を呼び出すとAEffect *というVSTプラグインの
	// Cインターフェースのオブジェクトが返る。
	typedef AEffect* (VstPluginEntryProc)(audioMasterCallback _callback);

	VstPlugin(LPCSTR _modulePath, size_t _samplingRate, size_t _blockSize, HostApplication* _host)
		: module_(_modulePath)
		,host_(_host)
		//,events_(0)
	{
		if (!module_) { throw std::runtime_error("module not found"); }
		initialize(_samplingRate, _blockSize);
		directory_ = module_.GetPath();
	}

	VstPlugin(const VstPlugin& _src) {}

	~VstPlugin()
	{
		terminate();
	}

public:
	AEffect* GetEffect() { return effect_; }
	AEffect* GetEffect() const { return effect_; }

	bool	IsSynth() const { return (effect_->flags & effFlagsIsSynth) != 0; }
	bool	HasEditor() const { return (effect_->flags & effFlagsHasEditor) != 0; }


	// AEffect *というプラグインのCインターフェースオブジェクトを経由して、
	// 実際のVSTプラグイン本体に命令を投げるには、
	// _opcodeとそれに付随するパラメータを渡して、
	// dispatcherという関数を呼び出す。
	VstIntPtr dispatcher(VstInt32 _opcode, VstInt32 _index, VstIntPtr _value, void* _ptr, float _opt) {
		return effect_->dispatcher(effect_, _opcode, _index, _value, _ptr, _opt);
	}

	VstIntPtr dispatcher(VstInt32 _opcode, VstInt32 _index, VstIntPtr _value, void* _ptr, float _opt) const {
		return effect_->dispatcher(effect_, _opcode, _index, _value, _ptr, _opt);
	}

	HostApplication& GetHost() { return *host_; }
	HostApplication const& GetHost() const { return *host_; }
	std::string	GetEffectName() const { return effectName_; }
	char const* GetDirectory() const { return directory_.c_str(); }

	size_t GetProgram() const { dispatcher(effGetProgram, 0, 0, 0, 0); }
	void SetProgram(size_t _index) { dispatcher(effSetProgram, 0, _index, 0, 0); }
	size_t GetNumPrograms() const { return effect_->numPrograms; }
	std::string GetProgramName(size_t _index) { return programNames_[_index]; }

#if false
	// ノートオンを受け取る
	// 実際のリアルタイム音楽アプリケーションでは、
	// ここでノート情報だけはなくさまざまなMIDI情報を
	// 正確なタイミングで記録するようにする。
	// 簡単のため、このアプリケーションでは、
	// ノート情報を随時コンテナに追加し、
	// 次の合成処理のタイミングで内部VSTプラグイン
	// にデータが送られることを期待する実装になっている。
	void AddNoteOn(size_t note_number)
	{
		VstMidiEvent event;
		event.type = kVstMidiType;
		event.byteSize = sizeof(VstMidiEvent);
		event.flags = kVstMidiEventIsRealtime;
		event.midiData[0] = static_cast<char>(0x90u);		// note on for 1st channel
		event.midiData[1] = static_cast<char>(note_number);	// note number
		event.midiData[2] = static_cast<char>(0x64u);		// velocity
		event.midiData[3] = 0;				// unused
		event.noteLength = 0;
		event.noteOffset = 0;
		event.detune = 0;
		event.deltaFrames = 0;
		event.noteOffVelocity = 100;
		event.reserved1 = 0;
		event.reserved2 = 0;

		auto lock = GetEventBufferLock();
		midiEvents_.push_back(event);
	}

	// ノートオンと同じ。
	void AddNoteOff(size_t note_number)
	{
		VstMidiEvent event;
		event.type = kVstMidiType;
		event.byteSize = sizeof(VstMidiEvent);
		event.flags = kVstMidiEventIsRealtime;
		event.midiData[0] = static_cast<char>(0x80u);		// note on for 1st channel
		event.midiData[1] = static_cast<char>(note_number);	// note number
		event.midiData[2] = static_cast<char>(0x64u);		// velocity
		event.midiData[3] = 0;				// unused
		event.noteLength = 0;
		event.noteOffset = 0;
		event.detune = 0;
		event.deltaFrames = 0;
		event.noteOffVelocity = 100;
		event.reserved1 = 0;
		event.reserved2 = 0;

		auto lock = GetEventBufferLock();
		midiEvents_.push_back(event);
	}


	// オーディオの合成処理に先立ち、
	// MIDI情報をVSTプラグイン本体に送る。
	// この処理は下のProcesAudioと同期的に行われるべき。
	// つまり、送信するべきイベントがある場合は、
	// ProcessAudioの直前に一度だけこの関数が呼ばれるようにする。
	void ProcessEvents()
	{
		{
			auto lock = GetEventBufferLock();
			// 送信用データをVstPlugin内部のバッファに移し替え。
			std::swap(tmp_, midiEvents_);
		}

		// 送信データがなにも無ければ返る。
		if (tmp_.empty()) { return; }

		// VstEvents型は、内部の配列を可変長配列として扱うので、
		// 送信したいMIDIイベントの数に合わせてメモリを確保
		// VstEvents型に、もともとVstEventのポインタ二つ分の領域が含まれているので、
		// 実際に確保するメモリ量は送信するイベント数から二つ引いたもので計算している。
		//!
		// ここで確保したメモリは
		// processReplacingが呼び出された後で解放する。
		size_t const bytes = sizeof(VstEvents) + sizeof(VstEvent*) * std::max<size_t>(tmp_.size(), 2) - 2;
		events_ = (VstEvents*)malloc(bytes);
		for (size_t i = 0; i < tmp_.size(); ++i) {
			events_->events[i] = reinterpret_cast<VstEvent*>(&tmp_[i]);
		}
		events_->numEvents = tmp_.size();
		events_->reserved = 0;

		// イベントを送信。
		dispatcher(effProcessEvents, 0, 0, events_, 0);
	}
#endif

	// オーディオ合成処理
	float** ProcessAudio(size_t frame)
	{
		assert(frame <= outputBuffers_[0].size());

		// 入力バッファ、出力バッファ、合成するべきサンプル時間を渡して
		// processReplacingを呼び出す。
		effect_->processReplacing(effect_, inputBufferHeads_.data(), outputBufferHeads_.data(), frame);

		// 合成終了なので
		// effProcessEventsで送信したデータを解放する。
		//tmp_.clear();
		//free(events_);
		//events_ = nullptr;

		return outputBufferHeads_.data();
	}

private:
	// プラグインの初期化処理
	void initialize(size_t _samplingRate, size_t _blockSize)
	{
		// エントリポイント取得
		VstPluginEntryProc* proc = module_.GetFunction<VstPluginEntryProc>("VSTPluginMain");
		if (!proc) {
			// 古いタイプのVSTプラグインでは、
			// エントリポイント名が"main"の場合がある。
			proc = module_.GetFunction<VstPluginEntryProc>("main");
			if (!proc) { throw std::runtime_error("entry point not found"); }
		}

		AEffect* test = proc(&VstHostCallback);
		if (!test || test->magic != kEffectMagic) { throw std::runtime_error("not a _vst plugin"); }

		effect_ = test;
		// このアプリケーションでAEffect *を扱いやすくするため
		// AEffectのユーザーデータ領域にこのクラスのオブジェクトのアドレスを
		// 格納しておく。
		effect_->user = this;

		// プラグインオープン
		dispatcher(effOpen, 0, 0, 0, 0);
		// 設定系
		dispatcher(effSetSampleRate, 0, 0, 0, static_cast<float>(_samplingRate));
		dispatcher(effSetBlockSize, 0, _blockSize, 0, 0.0);
		dispatcher(effSetProcessPrecision, 0, kVstProcessPrecision32, 0, 0);
		// プラグインの電源オン
		dispatcher(effMainsChanged, 0, true, 0, 0);
		// processReplacingが呼び出せる状態に
		dispatcher(effStartProcess, 0, 0, 0, 0);

		// プラグインの入力バッファ準備
		inputBuffers_.resize(effect_->numInputs);
		inputBufferHeads_.resize(effect_->numInputs);
		for (int i = 0; i < effect_->numInputs; ++i) {
			inputBuffers_[i].resize(_blockSize);
			inputBufferHeads_[i] = inputBuffers_[i].data();
		}

		// プラグインの出力バッファ準備
		outputBuffers_.resize(effect_->numOutputs);
		outputBufferHeads_.resize(effect_->numOutputs);
		for (int i = 0; i < effect_->numOutputs; ++i) {
			outputBuffers_[i].resize(_blockSize);
			outputBufferHeads_[i] = outputBuffers_[i].data();
		}

		// プラグイン名の取得
		std::array<char, kVstMaxEffectNameLen + 1> namebuf = {};
		dispatcher(effGetEffectName, 0, 0, namebuf.data(), 0);
		namebuf[namebuf.size() - 1] = '\0';
		effectName_ = namebuf.data();

		// プログラム(プラグインのパラメータのプリセット)リスト作成
		programNames_.resize(effect_->numPrograms);
		std::array<char, kVstMaxProgNameLen + 1> prognamebuf = {};
		for (int i = 0; i < effect_->numPrograms; ++i) {
			VstIntPtr result =
				dispatcher(effGetProgramNameIndexed, i, 0, prognamebuf.data(), 0);
			if (result) {
				prognamebuf[prognamebuf.size() - 1] = '\0';
				programNames_[i] = std::string(prognamebuf.data());
			}
			else {
				programNames_[i] = "unknown";
			}
		}
	}

	// 終了処理
	void terminate()
	{
		dispatcher(effStopProcess, 0, 0, 0, 0);
		dispatcher(effMainsChanged, 0, false, 0, 0);
		dispatcher(effClose, 0, 0, 0, 0);
	}

private:
	HostApplication* host_;
	Module module_;
	AEffect* effect_;

	std::vector<std::vector<float>>	outputBuffers_;
	std::vector<std::vector<float>> inputBuffers_;
	std::vector<float*>			outputBufferHeads_;
	std::vector<float*>			inputBufferHeads_;
	std::mutex mutable			eventBufferMutex_;
	//std::vector<VstMidiEvent>		midiEvents_;
	std::string						effectName_;
	std::string						directory_;
	std::vector<std::string>		programNames_;
	//std::vector<VstMidiEvent> tmp_;
	//VstEvents* events_;

	std::unique_lock<std::mutex> GetEventBufferLock() const { 
		return std::unique_lock<std::mutex>(eventBufferMutex_); 
	}
};



