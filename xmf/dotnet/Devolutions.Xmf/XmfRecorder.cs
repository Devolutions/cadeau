using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Devolutions.Xmf
{
    [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    [Guid("900598B4-0844-4DDE-8169-4AFEBB78FD77")]
    public interface IXmfRecorder
    {
        [MethodImpl(MethodImplOptions.PreserveSig)]
        bool Init();
        [MethodImpl(MethodImplOptions.PreserveSig)]
        void Uninit();
        [MethodImpl(MethodImplOptions.PreserveSig)]
        void SetFilename([MarshalAs(UnmanagedType.LPStr)] string filename);
        [MethodImpl(MethodImplOptions.PreserveSig)]
        void SetFrameSize(UInt32 frameWidth, UInt32 frameHeight);
        [MethodImpl(MethodImplOptions.PreserveSig)]
        UInt32 GetFrameRate();
        [MethodImpl(MethodImplOptions.PreserveSig)]
        void SetFrameRate(UInt32 frameRate);
        [MethodImpl(MethodImplOptions.PreserveSig)]
        void SetVideoQuality(UInt32 videoQuality);
        [MethodImpl(MethodImplOptions.PreserveSig)]
        UInt32 GetTimeout();
        [MethodImpl(MethodImplOptions.PreserveSig)]
        void Timeout();
        [MethodImpl(MethodImplOptions.PreserveSig)]
        void UpdateFrame(IntPtr buffer, UInt32 updateX, UInt32 updateY,
            UInt32 updateWidth, UInt32 updateHeight, UInt32 surfaceStep);
    }

    public static class XmfApi
    {
        private static Guid IID_IXmfRecorder =
            new Guid(0x900598B4, 0x0844, 0x4DDE, 0x81, 0x69, 0x4A, 0xFE, 0xBB, 0x78, 0xFD, 0x77);

        public static IXmfRecorder CreateRecorder()
        {
            object instance = null;
            XmfClassFactory_CreateInstance(null, ref IID_IXmfRecorder, out instance);
            return (IXmfRecorder)instance;
        }

        [DllImport("libxmf.dll")]
        public static extern uint XmfClassFactory_CreateInstance(
            [MarshalAs(UnmanagedType.IUnknown)] object pUnkOuter,
            ref Guid riid,
            [MarshalAs(UnmanagedType.IUnknown)] out object ppvObject);
    }
}
