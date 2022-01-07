using System;
using System.IO;
using System.Runtime.InteropServices;

using Devolutions.Xmf;

namespace Devolutions.XmfCli
{
    class Program
    {
        static void TestRecorder()
        {
            IXmfRecorder recorder = XmfApi.CreateRecorder();
            string tempPath = Path.GetTempPath();
            string filename = Path.Combine(tempPath, "video.webm");
            Console.WriteLine("{}", filename);
            recorder.SetFilename(filename);
            recorder.SetFrameSize(1024, 768);
            recorder.SetFrameRate(25);
            UInt32 frameRate = recorder.GetFrameRate();
            recorder.Init();
            UInt32 timeout = recorder.GetTimeout();

            UInt32 surfaceWidth = 1024;
            UInt32 surfaceHeight = 768;
            UInt32 surfaceStep = surfaceWidth * 4;
            int bufferSize = (int)(surfaceHeight * surfaceStep);
            IntPtr buffer = Marshal.AllocHGlobal(bufferSize);

            for (int i = 0; i < 100; i++)
            {
                for (int j = 0; j < bufferSize; j++)
                {
                    Marshal.WriteByte(buffer, i, (byte) (i * 2));
                }

                recorder.UpdateFrame(buffer, 0, 0, surfaceWidth, surfaceHeight, surfaceStep);
                recorder.Timeout();
                System.Threading.Thread.Sleep(100);
            }

            recorder.Uninit();
        }

        static void Main(string[] args)
        {
            TestRecorder();
        }
    }
}
