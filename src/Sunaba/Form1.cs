using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace SunabaIdeUnited
{
    public partial class Form1 : Form
    {
        //描画側システム
        private SunabaLib.SunabaSystem mSunabaSystem;
        private int mPointerX = 0;
        private int mPointerY = 0;
        private sbyte[] mKeys;
        private int mScale = 3;
        private int mWidth = 1;
        private int mHeight = 1;
        //クライアント
        private string mSourceFilename;
        //ウィンドウ状態
        private bool mEnlarging;
        private bool mPausing; //TODO:デバッグブレークが入れば無用になるかもしれぬ
        //言語
        private string mLangName;
        public Form1(string langName)
        {
            mLangName = langName;
            InitializeComponent();
            mEnlarging = false;
            mPausing = false;
            mScreenPictureBox.ClientSize = new System.Drawing.Size(0, 0);
            mScreenPictureBox.BackColor = Color.Green;//TODO:デバグ用 
            mScreenPictureBox.ForeColor = Color.Red;//TODO:デバグ用

            mSunabaSystem = new SunabaLib.SunabaSystem(this.Handle, mLangName);
            mSunabaSystem.setPictureBoxHandle(mScreenPictureBox.Handle, mWidth * mScale, mHeight * mScale);
            mScreenPictureBox.Refresh();
            mKeys = new sbyte[(int)SunabaLib.SunabaSystem.Key.KEY_COUNT];
            //言語ごとにボタンのテキストを変更
            if (mLangName == "chinese")
            {
                mRebootButton.Text = "重启";
                mEnlargeButton.Text = "放大/缩小";
            }
            else if (mLangName == "korean")
            {
                mRebootButton.Text = "재시작";
                mEnlargeButton.Text = "확대/축소";
            }
        }
        public void updateFrame()
        {
            int oldScale = mScale;
            if (!mPausing)
            {
                char[] message = new char[1];
                mSunabaSystem.update(ref message, mPointerX, mPointerY, mKeys);
                //向こうから情報が送られてきていれば、それをどうにかする。
                //現状はコマンドは来ない。全てメッセージ。TODO:そのうちちゃんとしろ
                if (message.Length > 0)
                {
                    addMessage(new string(message));
                }

            }
            else
            {
                System.Threading.Thread.Sleep(1);
            }
            int fps = mSunabaSystem.framePerSecond();
            int ratio = mSunabaSystem.calculationTimePercent();
            if (mLangName == "chinese")
            {
                mPerformanceLabel.Text = "每秒显示" + fps.ToString() + "次 负荷 " + ratio + "%";
            }
            else if (mLangName == "korean")
            {
                mPerformanceLabel.Text = "초당 " + fps.ToString() + "회 표시 부하 " + ratio + "%";
            }
            else
            {
                mPerformanceLabel.Text = "秒間" + fps.ToString() + "回表示 負荷" + ratio + "%";
            }
            if (
            (mSunabaSystem.screenWidth() != mWidth) ||
            (mSunabaSystem.screenHeight() != mHeight))
            {
                mWidth = mSunabaSystem.screenWidth();
                mHeight = mSunabaSystem.screenHeight();
                resize();
            }
            //ボタンの有効無効
            mRebootButton.Enabled = (mSourceFilename != null);
        }
        private void resize()
        {
            mEnlarging = true;
            //ピクチャボックスのサイズを変更
            mScreenPictureBox.ClientSize = new System.Drawing.Size(
                mWidth * mScale,
                mHeight * mScale);
            //フォームのサイズを、ピクチャボックスの右端、下端、メッセージの下端を元に設定
            int scEndX = mScreenPictureBox.Size.Width + mScreenPictureBox.Location.X;
            int scEndY = mScreenPictureBox.Size.Height + mScreenPictureBox.Location.Y;
            int mesEndY = mMessageWindowTextBox.Size.Height + mMessageWindowTextBox.Location.Y;
            int newCW = scEndX;
            int newCH = (scEndY > mesEndY) ? scEndY : mesEndY;
            this.Size = this.SizeFromClientSize(new Size(newCW + 12, newCH + 12));
            //メッセージボックスの高さを微調整
            int newMesEndY = this.ClientSize.Height - 12;
            int diffY = newMesEndY - mesEndY;
            Size oldMesSize = mMessageWindowTextBox.Size;
            mMessageWindowTextBox.Size = new Size(oldMesSize.Width, oldMesSize.Height + diffY);
            mEnlarging = false;
            mSunabaSystem.setPictureBoxHandle(mScreenPictureBox.Handle, mWidth * mScale, mHeight * mScale);
            mScreenPictureBox.Refresh();
        }
        private void addMessage(string s)
        {
            s = s.Replace("\n", "\r\n");
            mMessageWindowTextBox.Text += s;
            mMessageWindowTextBox.SelectionStart = mMessageWindowTextBox.Text.Length;
            mMessageWindowTextBox.ScrollToCaret();
        }
        public void bootProgram(string filename)
        {
            mSourceFilename = filename;
            //TODO:テキストボックスにファイル名表示
            load();
            Activate();
        }
        private void load()
        {
            //ロード時のキー状態を取得
            if (mSourceFilename != null)
            {
                UInt32[] instructions = new UInt32[1];
                char[] messageOut = new char[1];
                bool succeeded = SunabaLib.Compiler.compile(ref instructions, ref messageOut, mSourceFilename, mLangName);
                //メッセージ出力
                addMessage(new string(messageOut));

                if (succeeded) //オブジェクトコード送信
                {
                    //TODO:起動
                    //コード送信
                    byte[] data = new byte[(instructions.Length * 4)];
                    int pos = 0;
                    for (int i = 0; i < instructions.Length; ++i)
                    {
                        data[pos + 0] = (byte)(instructions[i] >> 24);
                        data[pos + 1] = (byte)(instructions[i] >> 16);
                        data[pos + 2] = (byte)(instructions[i] >> 8);
                        data[pos + 3] = (byte)(instructions[i] >> 0);
                        pos += 4;
                    }
                    mSunabaSystem.bootProgram(data);
                }
            }
        }
        private void mRebootButton_Click(object sender, EventArgs e)
        {
            load();
        }

        private void mRebootButton_PreviewKeyDown(object sender, PreviewKeyDownEventArgs e)
        {
            e.IsInputKey = true;
        }

        private void mEnlargeButton_Click(object sender, EventArgs e)
        {
            ++mScale;
            if (mScale == 6)
            {
                mScale = 1;
            }
            resize();
        }

        private void Form2_KeyDown(object sender, KeyEventArgs e)
        {
            switch (e.KeyCode)
            {
                case Keys.Up: mKeys[(int)SunabaLib.SunabaSystem.Key.KEY_UP] = 1; break;
                case Keys.Down: mKeys[(int)SunabaLib.SunabaSystem.Key.KEY_DOWN] = 1; break;
                case Keys.Left: mKeys[(int)SunabaLib.SunabaSystem.Key.KEY_LEFT] = 1; break;
                case Keys.Right: mKeys[(int)SunabaLib.SunabaSystem.Key.KEY_RIGHT] = 1; break;
                case Keys.Space: mKeys[(int)SunabaLib.SunabaSystem.Key.KEY_SPACE] = 1; break;
                case Keys.Enter: mKeys[(int)SunabaLib.SunabaSystem.Key.KEY_ENTER] = 1; break;
                case Keys.Pause: mPausing = !mPausing; break; //ポーズ
                case Keys.Delete: mMessageWindowTextBox.Text = ""; break;
            }
            e.Handled = true;

        }

        private void Form2_KeyUp(object sender, KeyEventArgs e)
        {
            switch (e.KeyCode)
            {
                case Keys.Up: mKeys[(int)SunabaLib.SunabaSystem.Key.KEY_UP] = 0; break;
                case Keys.Down: mKeys[(int)SunabaLib.SunabaSystem.Key.KEY_DOWN] = 0; break;
                case Keys.Left: mKeys[(int)SunabaLib.SunabaSystem.Key.KEY_LEFT] = 0; break;
                case Keys.Right: mKeys[(int)SunabaLib.SunabaSystem.Key.KEY_RIGHT] = 0; break;
                case Keys.Space: mKeys[(int)SunabaLib.SunabaSystem.Key.KEY_SPACE] = 0; break;
                case Keys.Enter: mKeys[(int)SunabaLib.SunabaSystem.Key.KEY_ENTER] = 0; break;
            }
            e.Handled = true;

        }

        private void Form2_DragDrop(object sender, DragEventArgs e)
        {
            if (e.Data.GetDataPresent(DataFormats.FileDrop))
            {
                //放りこまれたものを全部実行する。終了は待たない。これで文法チェックくらいはできる。
                string[] s = (string[])e.Data.GetData(DataFormats.FileDrop, false);
                foreach (string filename in s)
                {
                    mSourceFilename = filename;
                    //TODO:テキストボックスにファイル名表示
                    load();
                    Activate();
                }
            }
        }

        private void Form2_DragEnter(object sender, DragEventArgs e)
        {
            if (e.Data.GetDataPresent(DataFormats.FileDrop))
            {
                e.Effect = DragDropEffects.Copy;
            }
            else
            {
                e.Effect = DragDropEffects.None;
            }
        }

        private void Form2_MouseDown(object sender, MouseEventArgs e)
        {
            switch (e.Button)
            {
                case System.Windows.Forms.MouseButtons.Left: mKeys[(int)SunabaLib.SunabaSystem.Key.KEY_LBUTTON] = 1; break;
                case System.Windows.Forms.MouseButtons.Right: mKeys[(int)SunabaLib.SunabaSystem.Key.KEY_RBUTTON] = 1; break;
            }
        }

        private void Form2_MouseMove(object sender, MouseEventArgs e)
        {
            int realW = mScreenPictureBox.ClientSize.Width;
            int realH = mScreenPictureBox.ClientSize.Height;
            if ((realW != 0) && (realH != 0))
            {
                mPointerX = (mWidth * e.X) / realW;
                mPointerY = (mHeight * e.Y) / realH;
            }
        }

        private void Form2_MouseUp(object sender, MouseEventArgs e)
        {
            switch (e.Button)
            {
                case System.Windows.Forms.MouseButtons.Left: mKeys[(int)SunabaLib.SunabaSystem.Key.KEY_LBUTTON] = 0; break;
                case System.Windows.Forms.MouseButtons.Right: mKeys[(int)SunabaLib.SunabaSystem.Key.KEY_RBUTTON] = 0; break;
            }
        }

        private void Form2_FormClosing(object sender, FormClosingEventArgs e)
        {
            mSunabaSystem.end();
            mSunabaSystem = null;
        }
        private void Form2_Resize(object sender, EventArgs e)
        {
            if (mEnlarging)
            {
                return;
            }

            Control c = (Control)sender;
            int newH = c.ClientSize.Height;
            int newW = c.ClientSize.Width;
            Point pp = mScreenPictureBox.Location;
            Size ps = mScreenPictureBox.Size;
            Point mp = mMessageWindowTextBox.Location;
            Size ms = mMessageWindowTextBox.Size;
            int newMsH = newH - mp.Y - 12;
            int newMsW = newW - 12 - ps.Width - 6 - 12;
            mMessageWindowTextBox.Size = new Size(newMsW, newMsH);
            int newPbX = newW - 12 - ps.Width;
            int newPbY = 12;
            mScreenPictureBox.Location = new Point(newPbX, newPbY);
        }
    }
}
