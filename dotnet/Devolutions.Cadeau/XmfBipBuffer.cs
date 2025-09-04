using Microsoft.Win32.SafeHandles;
using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Devolutions.Cadeau
{
    public sealed class XmfBipBufferHandle : SafeHandleZeroOrMinusOneIsInvalid
    {
        private XmfBipBufferHandle() : base(ownsHandle: true) { }

        protected override bool ReleaseHandle()
        {
            Ffi.Free(handle);
            return true;
        }

        internal static class Ffi
        {
            private const string Lib = "xmf";

            [DllImport(Lib, EntryPoint = "XmfBipBuffer_New")]
            internal static extern XmfBipBufferHandle New(nuint size);

            [DllImport(Lib, EntryPoint = "XmfBipBuffer_Free")]
            internal static extern void Free(IntPtr h);
        }
    }

    public class XmfBipBuffer : IDisposable
    {
        private readonly XmfBipBufferHandle h;

        private bool disposed;

        private class Ffi
        {
            private const string Lib = "xmf";

            [DllImport(Lib, EntryPoint="XmfBipBuffer_Read")]
            public static extern int Read(XmfBipBufferHandle ffh, IntPtr data, nuint size);

            [DllImport(Lib, EntryPoint="XmfBipBuffer_Write")]
            public static extern int Write(XmfBipBufferHandle ffh, IntPtr data, nuint size);

            [DllImport(Lib, EntryPoint="XmfBipBuffer_UsedSize")]
            public static extern nuint GetUsedSize(XmfBipBufferHandle ffh);

            [DllImport(Lib, EntryPoint="XmfBipBuffer_BufferSize")]
            public static extern nuint GetBufferSize(XmfBipBufferHandle ffh);

            [DllImport(Lib, EntryPoint="XmfBipBuffer_GetSignaledState")]
            [return: MarshalAs(UnmanagedType.U1)]
            public static extern bool GetSignaledState(XmfBipBufferHandle ffh);

            [DllImport(Lib, EntryPoint="XmfBipBuffer_Grow")]
            [return: MarshalAs(UnmanagedType.U1)]
            public static extern bool Grow(XmfBipBufferHandle ffh, nuint size);

            [DllImport(Lib, EntryPoint="XmfBipBuffer_Clear")]
            public static extern void Clear(XmfBipBufferHandle ffh);
        }

        public XmfBipBufferHandle Handle => this.h;

        public XmfBipBuffer(nuint size)
        {
            this.h = XmfBipBufferHandle.Ffi.New(size) ?? throw new InvalidOperationException("XmfBipBuffer_New failed");

            if (this.h.IsInvalid)
            {
                throw new InvalidOperationException("Invalid XmfBipBuffer handle");
            }
        }

        public XmfBipBuffer() : this((nuint)(1024u * 1024u * 16u)) {}

        public void Dispose()
        {
            if (this.disposed)
            {
                return;
            }

            this.disposed = true;

            this.h?.Dispose();
            GC.SuppressFinalize(this);
        }

        public int Read(IntPtr data, nuint size)
        {
            this.CheckDisposed();

            return Ffi.Read(this.h, data, size);
        }

        public int Write(IntPtr data, nuint size)
        {
            this.CheckDisposed();

            return Ffi.Write(this.h, data, size);
        }

        public nuint GetUsedSize()
        {
            this.CheckDisposed();

            return Ffi.GetUsedSize(this.h);
        }

        public nuint GetBufferSize()
        {
            this.CheckDisposed();

            return Ffi.GetBufferSize(this.h);
        }

        public bool GetSignaledState()
        {
            this.CheckDisposed();

            return Ffi.GetSignaledState(this.h);
        }

        public bool Grow(nuint size)
        {
            this.CheckDisposed();

            return Ffi.Grow(this.h, size);
        }

        public void Clear()
        {
            this.CheckDisposed();

            Ffi.Clear(this.h);
        }

        private void CheckDisposed()
        {
            if (this.disposed)
            {
                throw new ObjectDisposedException(nameof(XmfBipBuffer));
            }
        }
    }
}
