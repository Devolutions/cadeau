using System;
using System.Runtime.InteropServices;

namespace Devolutions.Cadeau
{
    public class XmfMuxer
    {
        private IntPtr muxerHandle;

        private static class ffi
        {
            [DllImport("xmf", EntryPoint = "XmfWebMMuxer_New")]
            public static extern IntPtr New();

            [DllImport("xmf", EntryPoint = "XmfWebMMuxer_Remux")]
            public static extern int Remux(IntPtr muxer, [MarshalAs(UnmanagedType.LPStr)] string inputPath, [MarshalAs(UnmanagedType.LPStr)] string outputPath);

            [DllImport("xmf", EntryPoint = "XmfWebMMuxer_Cleanup")]
            public static extern void Cleanup(IntPtr muxer);
        }

        public IntPtr Handle => muxerHandle;

        public XmfMuxer()
        {
            muxerHandle = ffi.New();
        }

        ~XmfMuxer()
        {
            ffi.Cleanup(muxerHandle);
        }

        public int Remux(string inputPath, string outputPath)
        {
            return ffi.Remux(muxerHandle, inputPath, outputPath);
        }

        public void Cleanup()
        {
            ffi.Cleanup(muxerHandle);
        }
    }
}
