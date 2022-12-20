using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Devolutions.Cadeau
{
    public class XmfBipBuffer
    {
        private IntPtr ffh;

        private class ffi
        {
            [DllImport("xmf", EntryPoint="XmfBipBuffer_Read")]
            public static extern int Read(IntPtr ffh, IntPtr data, nuint size);

            [DllImport("xmf", EntryPoint="XmfBipBuffer_Write")]
            public static extern int Write(IntPtr ffh, IntPtr data, nuint size);

            [DllImport("xmf", EntryPoint="XmfBipBuffer_UsedSize")]
            public static extern nuint GetUsedSize(IntPtr ffh);

            [DllImport("xmf", EntryPoint="XmfBipBuffer_BufferSize")]
            public static extern nuint GetBufferSize(IntPtr ffh);

            [DllImport("xmf", EntryPoint="XmfBipBuffer_GetSignaledState")]
            [return: MarshalAs(UnmanagedType.U1)]
            public static extern bool GetSignaledState(IntPtr ffh);

            [DllImport("xmf", EntryPoint="XmfBipBuffer_Grow")]
            [return: MarshalAs(UnmanagedType.U1)]
            public static extern bool Grow(IntPtr ffh, nuint size);

            [DllImport("xmf", EntryPoint="XmfBipBuffer_Clear")]
            public static extern void Clear(IntPtr ffh);

            [DllImport("xmf", EntryPoint="XmfBipBuffer_New")]
            public static extern IntPtr New(nuint size);

            [DllImport("xmf", EntryPoint="XmfBipBuffer_Free")]
            public static extern void Free(IntPtr ffh);
        }

        public IntPtr Handle { get { return ffh; } }

        public XmfBipBuffer(nuint size)
        {
            ffh = ffi.New(size);
        }

        public XmfBipBuffer()
        {
            ffh = ffi.New(1024*1024*16);
        }

        ~XmfBipBuffer()
        {
            ffi.Free(ffh);
        }

        public int Read(IntPtr data, nuint size)
        {
            return ffi.Read(ffh, data, size);
        }

        public int Write(IntPtr data, nuint size)
        {
            return ffi.Write(ffh, data, size);
        }

        public nuint GetUsedSize()
        {
            return ffi.GetUsedSize(ffh);
        }

        public nuint GetBufferSize()
        {
            return ffi.GetBufferSize(ffh);
        }

        public bool GetSignaledState()
        {
            return ffi.GetSignaledState(ffh);
        }

        public bool Grow(nuint size)
        {
            return ffi.Grow(ffh, size);
        }

        public void Clear()
        {
            ffi.Clear(ffh);
        }
    }
}
