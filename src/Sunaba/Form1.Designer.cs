namespace SunabaIdeUnited
{
    partial class Form1
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.mRebootButton = new System.Windows.Forms.Button();
            this.mEnlargeButton = new System.Windows.Forms.Button();
            this.mMessageWindowTextBox = new System.Windows.Forms.TextBox();
            this.mScreenPictureBox = new System.Windows.Forms.PictureBox();
            this.mPerformanceLabel = new System.Windows.Forms.Label();
            ((System.ComponentModel.ISupportInitialize)(this.mScreenPictureBox)).BeginInit();
            this.SuspendLayout();
            // 
            // mRebootButton
            // 
            this.mRebootButton.Location = new System.Drawing.Point(12, 12);
            this.mRebootButton.Name = "mRebootButton";
            this.mRebootButton.Size = new System.Drawing.Size(75, 23);
            this.mRebootButton.TabIndex = 0;
            this.mRebootButton.Text = "再起動";
            this.mRebootButton.UseVisualStyleBackColor = true;
            this.mRebootButton.PreviewKeyDown += new System.Windows.Forms.PreviewKeyDownEventHandler(this.mRebootButton_PreviewKeyDown);
            this.mRebootButton.Click += new System.EventHandler(this.mRebootButton_Click);
            // 
            // mEnlargeButton
            // 
            this.mEnlargeButton.Location = new System.Drawing.Point(93, 12);
            this.mEnlargeButton.Name = "mEnlargeButton";
            this.mEnlargeButton.Size = new System.Drawing.Size(75, 23);
            this.mEnlargeButton.TabIndex = 1;
            this.mEnlargeButton.Text = "拡大/縮小";
            this.mEnlargeButton.UseVisualStyleBackColor = true;
            this.mEnlargeButton.PreviewKeyDown += new System.Windows.Forms.PreviewKeyDownEventHandler(this.mRebootButton_PreviewKeyDown);
            this.mEnlargeButton.Click += new System.EventHandler(this.mEnlargeButton_Click);
            // 
            // mMessageWindowTextBox
            // 
            this.mMessageWindowTextBox.Location = new System.Drawing.Point(12, 41);
            this.mMessageWindowTextBox.Multiline = true;
            this.mMessageWindowTextBox.Name = "mMessageWindowTextBox";
            this.mMessageWindowTextBox.ReadOnly = true;
            this.mMessageWindowTextBox.ScrollBars = System.Windows.Forms.ScrollBars.Both;
            this.mMessageWindowTextBox.Size = new System.Drawing.Size(303, 209);
            this.mMessageWindowTextBox.TabIndex = 2;
            this.mMessageWindowTextBox.TabStop = false;
            this.mMessageWindowTextBox.WordWrap = false;
            // 
            // mScreenPictureBox
            // 
            this.mScreenPictureBox.Location = new System.Drawing.Point(321, 17);
            this.mScreenPictureBox.Name = "mScreenPictureBox";
            this.mScreenPictureBox.Size = new System.Drawing.Size(20, 20);
            this.mScreenPictureBox.TabIndex = 3;
            this.mScreenPictureBox.TabStop = false;
            this.mScreenPictureBox.MouseMove += new System.Windows.Forms.MouseEventHandler(this.Form2_MouseMove);
            this.mScreenPictureBox.MouseDown += new System.Windows.Forms.MouseEventHandler(this.Form2_MouseDown);
            this.mScreenPictureBox.MouseUp += new System.Windows.Forms.MouseEventHandler(this.Form2_MouseUp);
            // 
            // mPerformanceLabel
            // 
            this.mPerformanceLabel.AutoSize = true;
            this.mPerformanceLabel.Font = new System.Drawing.Font("ＭＳ ゴシック", 9F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(128)));
            this.mPerformanceLabel.Location = new System.Drawing.Point(174, 17);
            this.mPerformanceLabel.Name = "mPerformanceLabel";
            this.mPerformanceLabel.Size = new System.Drawing.Size(137, 12);
            this.mPerformanceLabel.TabIndex = 4;
            this.mPerformanceLabel.Text = "秒間XXXX回表示 負荷XX%";
            // 
            // Form1
            // 
            this.AllowDrop = true;
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 12F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(353, 258);
            this.Controls.Add(this.mPerformanceLabel);
            this.Controls.Add(this.mScreenPictureBox);
            this.Controls.Add(this.mMessageWindowTextBox);
            this.Controls.Add(this.mEnlargeButton);
            this.Controls.Add(this.mRebootButton);
            this.KeyPreview = true;
            this.Name = "Form1";
            this.Text = "Sunaba";
            this.MouseUp += new System.Windows.Forms.MouseEventHandler(this.Form2_MouseUp);
            this.DragDrop += new System.Windows.Forms.DragEventHandler(this.Form2_DragDrop);
            this.MouseDown += new System.Windows.Forms.MouseEventHandler(this.Form2_MouseDown);
            this.DragEnter += new System.Windows.Forms.DragEventHandler(this.Form2_DragEnter);
            this.KeyUp += new System.Windows.Forms.KeyEventHandler(this.Form2_KeyUp);
            this.FormClosing += new System.Windows.Forms.FormClosingEventHandler(this.Form2_FormClosing);
            this.Resize += new System.EventHandler(this.Form2_Resize);
            this.MouseMove += new System.Windows.Forms.MouseEventHandler(this.Form2_MouseMove);
            this.KeyDown += new System.Windows.Forms.KeyEventHandler(this.Form2_KeyDown);
            ((System.ComponentModel.ISupportInitialize)(this.mScreenPictureBox)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Button mRebootButton;
        private System.Windows.Forms.Button mEnlargeButton;
        private System.Windows.Forms.TextBox mMessageWindowTextBox;
        private System.Windows.Forms.PictureBox mScreenPictureBox;
        private System.Windows.Forms.Label mPerformanceLabel;
    }
}