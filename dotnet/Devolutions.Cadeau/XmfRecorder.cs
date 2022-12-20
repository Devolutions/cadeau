using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Devolutions.Cadeau
{
    public class XmfRecorder
    {
        private IntPtr ffh;

        private class ffi
        {
            [DllImport("xmf", EntryPoint="XmfRecorder_Init")]
            [return: MarshalAs(UnmanagedType.U1)]
            public static extern bool Init(IntPtr ffh);

            [DllImport("xmf", EntryPoint="XmfRecorder_Uninit")]
            public static extern void Uninit(IntPtr ffh);

            [DllImport("xmf", EntryPoint="XmfRecorder_SetFileName")]
            public static extern void SetFileName(IntPtr ffh,
                [MarshalAs(UnmanagedType.LPStr)] string filename);

            [DllImport("xmf", EntryPoint="XmfRecorder_SetBipBuffer")]
            public static extern void SetBipBuffer(IntPtr ffh,
                IntPtr bb);

            [DllImport("xmf", EntryPoint="XmfRecorder_SetFrameSize")]
            public static extern void SetFrameSize(IntPtr ffh,
                UInt32 frameWidth, UInt32 frameHeight);

            [DllImport("xmf", EntryPoint="XmfRecorder_GetFrameRate")]
            public static extern UInt32 GetFrameRate(IntPtr ffh);

            [DllImport("xmf", EntryPoint="XmfRecorder_SetFrameRate")]
            public static extern void SetFrameRate(IntPtr ffh,
                UInt32 frameRate);

            [DllImport("xmf", EntryPoint="XmfRecorder_SetVideoQuality")]
            public static extern UInt32 SetVideoQuality(IntPtr ffh,
                UInt32 videoQuality);

            [DllImport("xmf", EntryPoint="XmfRecorder_SetCurrentTime")]
            public static extern void SetCurrentTime(IntPtr ffh,
                UInt64 currentTime);

            [DllImport("xmf", EntryPoint="XmfRecorder_GetCurrentTime")]
            public static extern UInt64 GetCurrentTime(IntPtr ffh);

            [DllImport("xmf", EntryPoint="XmfRecorder_GetTimeout")]
            public static extern UInt32 GetTimeout(IntPtr ffh);

            [DllImport("xmf", EntryPoint="XmfRecorder_Timeout")]
            public static extern void Timeout(IntPtr ffh);

            [DllImport("xmf", EntryPoint="XmfRecorder_UpdateFrame")]
            public static extern void UpdateFrame(IntPtr ffh,
                IntPtr buffer, UInt32 updateX, UInt32 updateY,
                UInt32 updateWidth, UInt32 updateHeight, UInt32 surfaceStep);

            [DllImport("xmf", EntryPoint="XmfRecorder_New")]
            public static extern IntPtr New();

            [DllImport("xmf", EntryPoint="XmfRecorder_Free")]
            public static extern void Free(IntPtr ffh);
        }

        public IntPtr Handle { get { return ffh; } }

        public XmfRecorder()
        {
            ffh = ffi.New();
        }

        ~XmfRecorder()
        {
            ffi.Free(ffh);
        }

        public bool Init()
        {
            return ffi.Init(ffh);
        }

        public void Uninit()
        {
            ffi.Uninit(ffh);
        }

        public void SetFileName(string filename)
        {
            ffi.SetFileName(ffh, filename);
        }

        public void SetBipBuffer(IntPtr bb)
        {
            ffi.SetBipBuffer(ffh, bb);
        }

        public void SetFrameSize(UInt32 frameWidth, UInt32 frameHeight)
        {
            ffi.SetFrameSize(ffh, frameWidth, frameHeight);
        }

        public UInt32 GetFrameRate()
        {
            return ffi.GetFrameRate(ffh);
        }

        public void SetFrameRate(UInt32 frameRate)
        {
            ffi.SetFrameRate(ffh, frameRate);
        }

        public void SetVideoQuality(UInt32 videoQuality)
        {
            ffi.SetVideoQuality(ffh, videoQuality);
        }

        public void SetCurrentTime(UInt64 currentTime)
        {
            ffi.SetCurrentTime(ffh, currentTime);
        }

        public UInt64 GetCurrentTime()
        {
            return ffi.GetCurrentTime(ffh);
        }

        public UInt32 GetTimeout()
        {
            return ffi.GetTimeout(ffh);
        }

        public void Timeout()
        {
            ffi.Timeout(ffh);
        }

        public void UpdateFrame(IntPtr buffer, UInt32 updateX, UInt32 updateY,
                UInt32 updateWidth, UInt32 updateHeight, UInt32 surfaceStep)
        {
            ffi.UpdateFrame(ffh, buffer, updateX, updateY, updateWidth, updateHeight, surfaceStep);
        }
    }
}
