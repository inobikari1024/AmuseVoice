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

// �v���O�C����������̗v���Ȃǂ��󂯂ČĂяo�����
// Host���̃R�[���o�b�N�֐�
VstIntPtr VSTCALLBACK VstHostCallback(AEffect* _effect, VstInt32 _opcode, VstInt32 _index, VstIntPtr _value, void* _ptr, float _opt);


// VST�v���O�C����\���N���X
struct VstPlugin
{
	// VST�v���O�C���̃G���g���|�C���g��\���֐��̌^
	// audioMasterCallback�Ƃ����z�X�g���̃R�[���o�b�N�֐���
	// �n���Ă��̊֐����Ăяo����AEffect *�Ƃ���VST�v���O�C����
	// C�C���^�[�t�F�[�X�̃I�u�W�F�N�g���Ԃ�B
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


	// AEffect *�Ƃ����v���O�C����C�C���^�[�t�F�[�X�I�u�W�F�N�g���o�R���āA
	// ���ۂ�VST�v���O�C���{�̂ɖ��߂𓊂���ɂ́A
	// _opcode�Ƃ���ɕt������p�����[�^��n���āA
	// dispatcher�Ƃ����֐����Ăяo���B
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
	// �m�[�g�I�����󂯎��
	// ���ۂ̃��A���^�C�����y�A�v���P�[�V�����ł́A
	// �����Ńm�[�g��񂾂��͂Ȃ����܂��܂�MIDI����
	// ���m�ȃ^�C�~���O�ŋL�^����悤�ɂ���B
	// �ȒP�̂��߁A���̃A�v���P�[�V�����ł́A
	// �m�[�g���𐏎��R���e�i�ɒǉ����A
	// ���̍��������̃^�C�~���O�œ���VST�v���O�C��
	// �Ƀf�[�^�������邱�Ƃ����҂�������ɂȂ��Ă���B
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

	// �m�[�g�I���Ɠ����B
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


	// �I�[�f�B�I�̍��������ɐ旧���A
	// MIDI����VST�v���O�C���{�̂ɑ���B
	// ���̏����͉���ProcesAudio�Ɠ����I�ɍs����ׂ��B
	// �܂�A���M����ׂ��C�x���g������ꍇ�́A
	// ProcessAudio�̒��O�Ɉ�x�������̊֐����Ă΂��悤�ɂ���B
	void ProcessEvents()
	{
		{
			auto lock = GetEventBufferLock();
			// ���M�p�f�[�^��VstPlugin�����̃o�b�t�@�Ɉڂ��ւ��B
			std::swap(tmp_, midiEvents_);
		}

		// ���M�f�[�^���Ȃɂ�������ΕԂ�B
		if (tmp_.empty()) { return; }

		// VstEvents�^�́A�����̔z����ϒ��z��Ƃ��Ĉ����̂ŁA
		// ���M������MIDI�C�x���g�̐��ɍ��킹�ă��������m��
		// VstEvents�^�ɁA���Ƃ���VstEvent�̃|�C���^����̗̈悪�܂܂�Ă���̂ŁA
		// ���ۂɊm�ۂ��郁�����ʂ͑��M����C�x���g���������������̂Ōv�Z���Ă���B
		//!
		// �����Ŋm�ۂ�����������
		// processReplacing���Ăяo���ꂽ��ŉ������B
		size_t const bytes = sizeof(VstEvents) + sizeof(VstEvent*) * std::max<size_t>(tmp_.size(), 2) - 2;
		events_ = (VstEvents*)malloc(bytes);
		for (size_t i = 0; i < tmp_.size(); ++i) {
			events_->events[i] = reinterpret_cast<VstEvent*>(&tmp_[i]);
		}
		events_->numEvents = tmp_.size();
		events_->reserved = 0;

		// �C�x���g�𑗐M�B
		dispatcher(effProcessEvents, 0, 0, events_, 0);
	}
#endif

	// �I�[�f�B�I��������
	float** ProcessAudio(size_t frame)
	{
		assert(frame <= outputBuffers_[0].size());

		// ���̓o�b�t�@�A�o�̓o�b�t�@�A��������ׂ��T���v�����Ԃ�n����
		// processReplacing���Ăяo���B
		effect_->processReplacing(effect_, inputBufferHeads_.data(), outputBufferHeads_.data(), frame);

		// �����I���Ȃ̂�
		// effProcessEvents�ő��M�����f�[�^���������B
		//tmp_.clear();
		//free(events_);
		//events_ = nullptr;

		return outputBufferHeads_.data();
	}

private:
	// �v���O�C���̏���������
	void initialize(size_t _samplingRate, size_t _blockSize)
	{
		// �G���g���|�C���g�擾
		VstPluginEntryProc* proc = module_.GetFunction<VstPluginEntryProc>("VSTPluginMain");
		if (!proc) {
			// �Â��^�C�v��VST�v���O�C���ł́A
			// �G���g���|�C���g����"main"�̏ꍇ������B
			proc = module_.GetFunction<VstPluginEntryProc>("main");
			if (!proc) { throw std::runtime_error("entry point not found"); }
		}

		AEffect* test = proc(&VstHostCallback);
		if (!test || test->magic != kEffectMagic) { throw std::runtime_error("not a _vst plugin"); }

		effect_ = test;
		// ���̃A�v���P�[�V������AEffect *�������₷�����邽��
		// AEffect�̃��[�U�[�f�[�^�̈�ɂ��̃N���X�̃I�u�W�F�N�g�̃A�h���X��
		// �i�[���Ă����B
		effect_->user = this;

		// �v���O�C���I�[�v��
		dispatcher(effOpen, 0, 0, 0, 0);
		// �ݒ�n
		dispatcher(effSetSampleRate, 0, 0, 0, static_cast<float>(_samplingRate));
		dispatcher(effSetBlockSize, 0, _blockSize, 0, 0.0);
		dispatcher(effSetProcessPrecision, 0, kVstProcessPrecision32, 0, 0);
		// �v���O�C���̓d���I��
		dispatcher(effMainsChanged, 0, true, 0, 0);
		// processReplacing���Ăяo�����Ԃ�
		dispatcher(effStartProcess, 0, 0, 0, 0);

		// �v���O�C���̓��̓o�b�t�@����
		inputBuffers_.resize(effect_->numInputs);
		inputBufferHeads_.resize(effect_->numInputs);
		for (int i = 0; i < effect_->numInputs; ++i) {
			inputBuffers_[i].resize(_blockSize);
			inputBufferHeads_[i] = inputBuffers_[i].data();
		}

		// �v���O�C���̏o�̓o�b�t�@����
		outputBuffers_.resize(effect_->numOutputs);
		outputBufferHeads_.resize(effect_->numOutputs);
		for (int i = 0; i < effect_->numOutputs; ++i) {
			outputBuffers_[i].resize(_blockSize);
			outputBufferHeads_[i] = outputBuffers_[i].data();
		}

		// �v���O�C�����̎擾
		std::array<char, kVstMaxEffectNameLen + 1> namebuf = {};
		dispatcher(effGetEffectName, 0, 0, namebuf.data(), 0);
		namebuf[namebuf.size() - 1] = '\0';
		effectName_ = namebuf.data();

		// �v���O����(�v���O�C���̃p�����[�^�̃v���Z�b�g)���X�g�쐬
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

	// �I������
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



