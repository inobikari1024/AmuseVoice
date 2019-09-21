using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.Runtime.InteropServices; // DLL 読み込みのため必要

using NAudio;
using NAudio.Wave;


namespace AmuseVoice
{

    public partial class Form1 : Form
    {
        bool isRecording;
        bool isMouseDown;
        WaveInEvent waveIn;
        WaveOut waveOut;
        BufferedWaveProvider waveProvider;

        public Form1()
        {
            InitializeComponent();
            this.Icon = Properties.Resources.app_icon;
            this.MaximizeBox = false;
            this.FormBorderStyle = FormBorderStyle.FixedSingle;
            playVoiceCheck.Checked = true;
            isRecording = false;
            isMouseDown = false;

            this.CheckDevice();

        }

        private void CheckDevice()
        {
            int count = WaveInEvent.DeviceCount;
            for (int i = 0; i < count; ++i) {
                string name = WaveInEvent.GetCapabilities(i).ProductName;
                inputDeviceBox.Items.Add(name);
            }

            count = WaveOut.DeviceCount;
            for (int i = 0; i < count; ++i) {
                string name = WaveOut.GetCapabilities(i).ProductName;
                outputDeviceBox.Items.Add(name);
            }
            inputDeviceBox.SelectedIndex = 0;
            outputDeviceBox.SelectedIndex = 0;
        }

        private void StartRecord()
        {
            waveIn = new WaveInEvent();
            int device = inputDeviceBox.SelectedIndex;
            int channels = WaveInEvent.GetCapabilities(device).Channels;
            waveIn.DeviceNumber = device;
            waveIn.WaveFormat = new WaveFormat(48000, channels);
            //waveIn.BufferMilliseconds = 100;
            //waveIn.NumberOfBuffers = 2;

            waveProvider = new BufferedWaveProvider(waveIn.WaveFormat);
            waveProvider.DiscardOnBufferOverflow = true;
            //waveProvider.BufferLength = 44100;
            //waveProvider.BufferDuration = TimeSpan.FromMilliseconds(10000);

            waveOut = new WaveOut();
            waveOut.Init(waveProvider);
            waveOut.DeviceNumber = outputDeviceBox.SelectedIndex;

            waveIn.DataAvailable += (_, e) => {
                waveProvider.AddSamples(e.Buffer, 0, e.BytesRecorded);
            };
            waveIn.RecordingStopped += (_, __) => {
                waveProvider.ClearBuffer();
            };

            waveProvider.ClearBuffer();
            waveIn.StartRecording();
            if (playVoiceCheck.Checked)
                waveOut.Play();
        }

        private void DoneRecord()
        {
            waveOut.Stop();
            waveIn.StopRecording();

            if (waveIn != null)
                waveIn.Dispose();
            if (waveOut != null)
                waveOut.Dispose();
        }

        private void Recording_Click(object sender, EventArgs e)
        {
            if (isRecording) {
                isRecording = false;
                recording.Text = "録音開始";
                recording.BackColor = Color.Transparent;
                this.DoneRecord();
            }
            else {
                isRecording = true;
                recording.Text = "録音停止";
                recording.BackColor = Color.FromArgb(255, 168, 168);
                this.StartRecord();
            }
        }

        private void PlayVoiceCheck_CheckedChanged(object sender, EventArgs e)
        {
            if (isRecording) {
                if (playVoiceCheck.Checked)
                    waveOut.Play();
                else
                    waveOut.Stop();
            }
        }



        [DllImport("PluginReader.dll")]
        extern static bool LoadPlugin(string vstPath);

        private void AddButton_Click(object sender, EventArgs e)
        {
            openFileDialog1.ShowDialog();
        }

        private void OpenFileDialog1_FileOk(object sender, CancelEventArgs e)
        {
            //LoadPlugin(openFileDialog1.FileName);
            if (vstList.Items.Contains(openFileDialog1.SafeFileName)) {
                return;
            }
            vstList.Items.Add(openFileDialog1.SafeFileName);
        }
    }
}
