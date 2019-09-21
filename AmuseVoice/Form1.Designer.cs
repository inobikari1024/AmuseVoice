namespace AmuseVoice
{
    partial class Form1
    {
        /// <summary>
        /// 必要なデザイナー変数です。
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// 使用中のリソースをすべてクリーンアップします。
        /// </summary>
        /// <param name="disposing">マネージド リソースを破棄する場合は true を指定し、その他の場合は false を指定します。</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows フォーム デザイナーで生成されたコード

        /// <summary>
        /// デザイナー サポートに必要なメソッドです。このメソッドの内容を
        /// コード エディターで変更しないでください。
        /// </summary>
        private void InitializeComponent()
        {
            this.recording = new System.Windows.Forms.Button();
            this.inputDeviceBox = new System.Windows.Forms.ComboBox();
            this.outputDeviceBox = new System.Windows.Forms.ComboBox();
            this.label1 = new System.Windows.Forms.Label();
            this.label2 = new System.Windows.Forms.Label();
            this.invalidCheck = new System.Windows.Forms.CheckBox();
            this.playVoiceCheck = new System.Windows.Forms.CheckBox();
            this.vstList = new System.Windows.Forms.ListBox();
            this.addButton = new System.Windows.Forms.Button();
            this.label3 = new System.Windows.Forms.Label();
            this.openFileDialog1 = new System.Windows.Forms.OpenFileDialog();
            this.SuspendLayout();
            // 
            // recording
            // 
            this.recording.Location = new System.Drawing.Point(645, 36);
            this.recording.Name = "recording";
            this.recording.Size = new System.Drawing.Size(200, 80);
            this.recording.TabIndex = 0;
            this.recording.Text = "録音開始";
            this.recording.UseVisualStyleBackColor = true;
            this.recording.Click += new System.EventHandler(this.Recording_Click);
            // 
            // inputDeviceBox
            // 
            this.inputDeviceBox.FormattingEnabled = true;
            this.inputDeviceBox.Location = new System.Drawing.Point(55, 68);
            this.inputDeviceBox.Name = "inputDeviceBox";
            this.inputDeviceBox.Size = new System.Drawing.Size(360, 32);
            this.inputDeviceBox.TabIndex = 1;
            this.inputDeviceBox.Text = "-デバイスを選択してください-";
            // 
            // outputDeviceBox
            // 
            this.outputDeviceBox.FormattingEnabled = true;
            this.outputDeviceBox.Location = new System.Drawing.Point(55, 172);
            this.outputDeviceBox.Name = "outputDeviceBox";
            this.outputDeviceBox.Size = new System.Drawing.Size(360, 32);
            this.outputDeviceBox.TabIndex = 2;
            this.outputDeviceBox.Text = "-デバイスを選択してください-";
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(52, 36);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(253, 24);
            this.label1.TabIndex = 3;
            this.label1.Text = "音声入力デバイス（録音）";
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(52, 140);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(253, 24);
            this.label2.TabIndex = 4;
            this.label2.Text = "音声出力デバイス（再生）";
            // 
            // invalidCheck
            // 
            this.invalidCheck.AutoSize = true;
            this.invalidCheck.Location = new System.Drawing.Point(504, 140);
            this.invalidCheck.Name = "invalidCheck";
            this.invalidCheck.Size = new System.Drawing.Size(196, 28);
            this.invalidCheck.TabIndex = 5;
            this.invalidCheck.Text = "エフェクト無効化";
            this.invalidCheck.UseVisualStyleBackColor = true;
            // 
            // playVoiceCheck
            // 
            this.playVoiceCheck.AutoSize = true;
            this.playVoiceCheck.Location = new System.Drawing.Point(504, 180);
            this.playVoiceCheck.Name = "playVoiceCheck";
            this.playVoiceCheck.Size = new System.Drawing.Size(321, 28);
            this.playVoiceCheck.TabIndex = 6;
            this.playVoiceCheck.Text = "自分の声を聞く（ループバック）";
            this.playVoiceCheck.UseVisualStyleBackColor = true;
            this.playVoiceCheck.CheckedChanged += new System.EventHandler(this.PlayVoiceCheck_CheckedChanged);
            // 
            // vstList
            // 
            this.vstList.FormattingEnabled = true;
            this.vstList.ItemHeight = 24;
            this.vstList.Location = new System.Drawing.Point(55, 301);
            this.vstList.Name = "vstList";
            this.vstList.Size = new System.Drawing.Size(709, 148);
            this.vstList.TabIndex = 7;
            // 
            // addButton
            // 
            this.addButton.Font = new System.Drawing.Font("MS UI Gothic", 10F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(128)));
            this.addButton.Location = new System.Drawing.Point(770, 409);
            this.addButton.Name = "addButton";
            this.addButton.Size = new System.Drawing.Size(75, 40);
            this.addButton.TabIndex = 8;
            this.addButton.Text = "+";
            this.addButton.UseVisualStyleBackColor = true;
            this.addButton.Click += new System.EventHandler(this.AddButton_Click);
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(52, 270);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(141, 24);
            this.label3.TabIndex = 4;
            this.label3.Text = "VSTプラグイン";
            // 
            // openFileDialog1
            // 
            this.openFileDialog1.FileName = "openFileDialog1";
            this.openFileDialog1.FileOk += new System.ComponentModel.CancelEventHandler(this.OpenFileDialog1_FileOk);
            // 
            // Form1
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(13F, 24F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(900, 940);
            this.Controls.Add(this.addButton);
            this.Controls.Add(this.vstList);
            this.Controls.Add(this.playVoiceCheck);
            this.Controls.Add(this.invalidCheck);
            this.Controls.Add(this.label3);
            this.Controls.Add(this.label2);
            this.Controls.Add(this.label1);
            this.Controls.Add(this.outputDeviceBox);
            this.Controls.Add(this.inputDeviceBox);
            this.Controls.Add(this.recording);
            this.Name = "Form1";
            this.Text = "AmuseVoice";
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Button recording;
        private System.Windows.Forms.ComboBox inputDeviceBox;
        private System.Windows.Forms.ComboBox outputDeviceBox;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.CheckBox invalidCheck;
        private System.Windows.Forms.CheckBox playVoiceCheck;
        private System.Windows.Forms.ListBox vstList;
        private System.Windows.Forms.Button addButton;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.OpenFileDialog openFileDialog1;
    }
}

