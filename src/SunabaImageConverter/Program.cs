using System;
using System.Text;

namespace SunabaImageConverter
{
    class Program
    {
        static void Main(string[] args)
        {
            if (args.Length == 0)
            {
                return;
            }
            System.Drawing.Bitmap image = new System.Drawing.Bitmap(args[0]);
            System.IO.StreamWriter writer = new System.IO.StreamWriter("out.txt", false, Encoding.GetEncoding("shift_jis"));
            int w = image.Width;
            int h = image.Height;
            if (w >= 100)
            {
                w = 100;
            }
            if (h >= 100)
            {
                h = 100;
            }
            for (int y = 0; y < h; ++y)
            {
                for (int x = 0; x < w; ++x)
                {
                    System.Drawing.Color c = image.GetPixel(x, y);
                    int r = c.R * 99 / 255;
                    int g = c.G * 99 / 255;
                    int b = c.B * 99 / 255;
                    writer.Write("メモリ[");
                    writer.Write((60000 + (y * 100) + x).ToString());
                    writer.Write("] → ");
                    writer.Write(((r * 10000) + (g * 100) + b).ToString());
                    writer.Write("\r\n");
                }
            }
            image.Dispose();
            writer.Dispose();
        }
    }
}
