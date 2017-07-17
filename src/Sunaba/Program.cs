using System;
using System.Collections.Generic;
using System.Windows.Forms;
using System.Runtime.InteropServices;

namespace SunabaIdeUnited
{
    static class Program
    {
        //select language
        private static string mLangName = "japanese";
//        private static string mLangName = "chinese";
//        private static string mLangName = "korean";

        [DllImport("user32.dll")]
        private static extern bool SetForegroundWindow(IntPtr hWnd);
        [DllImport("user32.dll")]
        private static extern bool ShowWindowAsync(IntPtr hWnd, int nCmdShow);
        private const int SW_RESTORE = 9;
        [DllImport("user32.dll")]
        private static extern bool IsIconic(IntPtr hWnd);
        /// アプリケーションのメイン エントリ ポイントです。
        /// </summary>
        [STAThread]
        static void Main()
        {
            //多重起動阻止。すでにいるものを起こす。
            string name = System.Diagnostics.Process.GetCurrentProcess().ProcessName;
            System.Diagnostics.Process[] processes = System.Diagnostics.Process.GetProcessesByName(name);
            if (processes.Length > 1)
            {
                IntPtr h = processes[0].MainWindowHandle;
                if (IsIconic(h))
                {
                    ShowWindowAsync(h, SW_RESTORE);
                }
                SetForegroundWindow(h);
                return;
            }
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);
            //起動ファイル
            string filename = null;
            if (Environment.GetCommandLineArgs().Length >= 2)
            {
                filename = Environment.GetCommandLineArgs()[1];
            }
            Form1 form = new Form1(mLangName);
            form.Show();
            while (form.Created)
            {
                if (filename != null) //起動ファイル
                {
                    form.bootProgram(filename);
                    filename = null;
                }
                form.updateFrame();
                Application.DoEvents();
            }
        }
    }
}
