using System;
using System.IO;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Devolutions.Cadeau
{
    public class XmfImage
    {
        private unsafe class ffi
        {
            [DllImport("xmf", EntryPoint="XmfImage_LoadFile")]
            [return: MarshalAs(UnmanagedType.U1)]
            public static extern bool LoadFile_ptr(
                [MarshalAs(UnmanagedType.LPStr)] string filename,
                ref IntPtr data, ref uint width, ref uint height, ref uint step);

            [DllImport("xmf", EntryPoint="XmfImage_LoadFile")]
            [return: MarshalAs(UnmanagedType.U1)]
            public static extern bool LoadFile_raw(
                [MarshalAs(UnmanagedType.LPStr)] string filename,
                ref byte* data, ref uint width, ref uint height, ref uint step);

            [DllImport("xmf", EntryPoint="XmfImage_SaveFile")]
            [return: MarshalAs(UnmanagedType.U1)]
            public static extern bool SaveFile_ptr(
                [MarshalAs(UnmanagedType.LPStr)] string filename,
                IntPtr data, uint width, uint height, uint step);

            [DllImport("xmf", EntryPoint="XmfImage_SaveFile")]
            [return: MarshalAs(UnmanagedType.U1)]
            public static extern bool SaveFile_raw(
                [MarshalAs(UnmanagedType.LPStr)] string filename,
                byte* data, uint width, uint height, uint step);

            [DllImport("xmf", EntryPoint="XmfImage_FreeData")]
            public static extern void FreeData(IntPtr data);
        }

        public static bool LoadFile(string filename, ref IntPtr data, ref uint width, ref uint height, ref uint step)
        {
            unsafe {
                return ffi.LoadFile_ptr(filename, ref data, ref width, ref height, ref step);
            }
        }

        public static bool SaveFile(string filename, IntPtr data, uint width, uint height, uint step)
        {
            unsafe {
                return ffi.SaveFile_ptr(filename, data, width, height, step);
            }
        }

        public static void FreeData(IntPtr data)
        {
            ffi.FreeData(data);
        }
    }
}
