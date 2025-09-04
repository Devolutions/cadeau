using Microsoft.Win32.SafeHandles;
using System;
using System.Runtime.InteropServices;

namespace Devolutions.Cadeau
{
    public sealed class XmfRecorderHandle : SafeHandleZeroOrMinusOneIsInvalid
    {
        private XmfRecorderHandle() : base(ownsHandle: true) { }

        protected override bool ReleaseHandle()
        {
            Ffi.Free(this.handle);
            return true;
        }

        internal static class Ffi
        {
            private const string Lib = "xmf";

            [DllImport(Lib, EntryPoint = "XmfRecorder_New")]
            internal static extern XmfRecorderHandle New();

            [DllImport(Lib, EntryPoint = "XmfRecorder_Free")]
            internal static extern void Free(IntPtr ffh);
        }
    }

    public class XmfRecorder : IDisposable
    {
        private readonly XmfRecorderHandle h;

        private readonly object gate = new();

        private bool disposed;

        private class Ffi
        {
            private const string Lib = "xmf";

            [DllImport(Lib, EntryPoint = "XmfRecorder_Init")]
            [return: MarshalAs(UnmanagedType.U1)]
            public static extern bool Init(XmfRecorderHandle ffh);

            [DllImport(Lib, EntryPoint = "XmfRecorder_Uninit")]
            public static extern void Uninit(XmfRecorderHandle ffh);

            [DllImport(Lib, EntryPoint = "XmfRecorder_SetFileName")]
            public static extern void SetFileName(XmfRecorderHandle ffh, [MarshalAs(UnmanagedType.LPStr)] string filename);

            [DllImport(Lib, EntryPoint = "XmfRecorder_SetBipBuffer")]
            public static extern void SetBipBuffer(XmfRecorderHandle ffh, IntPtr bb);

            [DllImport(Lib, EntryPoint = "XmfRecorder_SetFrameSize")]
            public static extern void SetFrameSize(XmfRecorderHandle ffh, uint frameWidth, uint frameHeight);

            [DllImport(Lib, EntryPoint = "XmfRecorder_GetFrameRate")]
            public static extern uint GetFrameRate(XmfRecorderHandle ffh);

            [DllImport(Lib, EntryPoint = "XmfRecorder_SetFrameRate")]
            public static extern void SetFrameRate(XmfRecorderHandle ffh, uint frameRate);

            [DllImport(Lib, EntryPoint = "XmfRecorder_SetVideoQuality")]
            public static extern uint SetVideoQuality(XmfRecorderHandle ffh,
                uint videoQuality);

            [DllImport(Lib, EntryPoint = "XmfRecorder_SetCurrentTime")]
            public static extern void SetCurrentTime(XmfRecorderHandle ffh,
                ulong currentTime);

            [DllImport(Lib, EntryPoint = "XmfRecorder_GetCurrentTime")]
            public static extern ulong GetCurrentTime(XmfRecorderHandle ffh);

            [DllImport(Lib, EntryPoint = "XmfRecorder_GetTimeout")]
            public static extern uint GetTimeout(XmfRecorderHandle ffh);

            [DllImport(Lib, EntryPoint = "XmfRecorder_Timeout")]
            public static extern void Timeout(XmfRecorderHandle ffh);

            [DllImport(Lib, EntryPoint = "XmfRecorder_UpdateFrame")]
            public static extern void UpdateFrame(XmfRecorderHandle ffh,
                IntPtr buffer, uint updateX, uint updateY,
                uint updateWidth, uint updateHeight, uint surfaceStep);
        }

        public XmfRecorderHandle Handle => this.h;

        public XmfRecorder()
        {
            this.h = XmfRecorderHandle.Ffi.New() ?? throw new InvalidOperationException("Failed to create XmfRecorderHandle");

            if (this.h.IsInvalid)
            {
                throw new InvalidOperationException("Invalid XmfRecorderHandle");
            }
        }

        private XmfRecorder(XmfRecorderHandle handle)
        {
            this.h = handle;
        }

        public static bool TryCreate(out XmfRecorder recorder)
        {
            recorder = null;

            XmfRecorderHandle handle;

            try
            {
                handle = XmfRecorderHandle.Ffi.New();
            }
            catch
            {
                return false;
            }

            if (handle == null || handle.IsInvalid)
            {
                handle?.Dispose();
                return false;
            }

            recorder = new XmfRecorder(handle);
            return true;
        }

        public void Dispose()
        {
            if (this.disposed)
            {
                return;
            }

            this.disposed = true;

            lock (this.gate)
            {
                try
                {
                    if (!this.h.IsInvalid)
                    {
                        Ffi.Uninit(this.h);
                    }
                }
                catch
                {
                    // miam
                }
            }

            this.h.Dispose();
            GC.SuppressFinalize(this);
        }

        public bool Init() => Call(Ffi.Init);

        public void Uninit() => Call(Ffi.Uninit);

        public void SetFileName(string filename) => Call(h => Ffi.SetFileName(h, filename));

        [Obsolete("Use SetBipBuffer with XmfBipBuffer instead")]
        public void SetBipBuffer(IntPtr bb) => Call(h => Ffi.SetBipBuffer(h, bb));

        public void SetBipBuffer(XmfBipBuffer bb) => Call(h => Ffi.SetBipBuffer(h, bb.Handle.DangerousGetHandle()));

        public void SetFrameSize(uint frameWidth, uint frameHeight) => Call(h => Ffi.SetFrameSize(h, frameWidth, frameHeight));

        public uint GetFrameRate() => Call(Ffi.GetFrameRate);


        public void SetFrameRate(uint frameRate) => Call(h => Ffi.SetFrameRate(h, frameRate));

        public void SetVideoQuality(uint videoQuality) => Call(h => Ffi.SetVideoQuality(h, videoQuality));

        public void SetCurrentTime(ulong currentTime) => Call(h => Ffi.SetCurrentTime(h, currentTime));

        public ulong GetCurrentTime() => Call(Ffi.GetCurrentTime);

        public uint GetTimeout() => Call(Ffi.GetTimeout);

        public void Timeout() => Call(Ffi.Timeout);

        public void UpdateFrame(IntPtr buffer, uint updateX, uint updateY,
                uint updateWidth, uint updateHeight, uint surfaceStep) => 
            Call(h => Ffi.UpdateFrame(h, buffer, updateX, updateY, updateWidth, updateHeight, surfaceStep));

        private T Call<T>(Func<XmfRecorderHandle, T> func)
        {
            this.CheckDisposed();

            lock (this.gate)
            {
                return func(this.h);
            }
        }

        private void Call(Action<XmfRecorderHandle> action)
        {
            this.CheckDisposed();

            lock (this.gate)
            {
                action(this.h);
            }
        }

        private void CheckDisposed()
        {
            if (this.disposed)
            {
                throw new ObjectDisposedException(nameof(XmfRecorder));
            }
        }
    }
}
