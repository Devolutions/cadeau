using Microsoft.Win32.SafeHandles;
using System;
using System.Runtime.InteropServices;

namespace Devolutions.Cadeau
{
    public enum XmfVpxImageFormat
    {
        None = 0,
        Yv12 = 0x301,
        I420 = 0x102,
        I422 = 0x105,
        I444 = 0x106,
        I440 = 0x107,
        Nv12 = 0x109,
        I42016 = 0x902,
        I42216 = 0x905,
        I44416 = 0x906,
        I44016 = 0x907,
    }

    public enum XmfVpxColorSpace
    {
        Unknown = 0,
        Bt601 = 1,
        Bt709 = 2,
        Smpte170 = 3,
        Smpte240 = 4,
        Bt2020 = 5,
        Reserved = 6,
        Srgb = 7,
    }

    public enum XmfVpxColorRange
    {
        Studio = 0,
        Full = 1,
    }

    public enum XmfVpxPlane
    {
        Y = 0,
        U = 1,
        V = 2,
        Alpha = 3,
    }

    public sealed class XmfVpxImageHandle : SafeHandleZeroOrMinusOneIsInvalid
    {
        private XmfVpxImageHandle() : base(ownsHandle: true) { }

        protected override bool ReleaseHandle()
        {
            Ffi.Destroy(handle);
            return true;
        }

        internal static class Ffi
        {
            private const string Lib = "xmf";

            [DllImport(Lib, EntryPoint = "XmfVpxImage_Destroy")]
            internal static extern void Destroy(IntPtr image);
        }
    }

    /// <summary>
    /// A frame returned by <see cref="XmfVpxDecoder.GetNextFrame"/>. Its planes point into decoder-owned
    /// memory that stays valid only until the next call on that decoder.
    /// </summary>
    public class XmfVpxImage : IDisposable
    {
        private readonly XmfVpxImageHandle h;

        private bool disposed;

        private class Ffi
        {
            private const string Lib = "xmf";

            [DllImport(Lib, EntryPoint = "XmfVpxImage_GetWidth")]
            public static extern uint GetWidth(XmfVpxImageHandle image);

            [DllImport(Lib, EntryPoint = "XmfVpxImage_GetHeight")]
            public static extern uint GetHeight(XmfVpxImageHandle image);

            [DllImport(Lib, EntryPoint = "XmfVpxImage_GetFormat")]
            public static extern int GetFormat(XmfVpxImageHandle image);

            [DllImport(Lib, EntryPoint = "XmfVpxImage_GetPlane")]
            public static extern IntPtr GetPlane(XmfVpxImageHandle image, int plane);

            [DllImport(Lib, EntryPoint = "XmfVpxImage_GetStride")]
            public static extern int GetStride(XmfVpxImageHandle image, int plane);

            [DllImport(Lib, EntryPoint = "XmfVpxImage_GetColorSpace")]
            public static extern int GetColorSpace(XmfVpxImageHandle image);

            [DllImport(Lib, EntryPoint = "XmfVpxImage_GetColorRange")]
            public static extern int GetColorRange(XmfVpxImageHandle image);
        }

        internal XmfVpxImage(XmfVpxImageHandle h)
        {
            this.h = h;
        }

        public XmfVpxImageHandle Handle => this.h;

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

        public uint GetWidth()
        {
            this.CheckDisposed();

            return Ffi.GetWidth(this.h);
        }

        public uint GetHeight()
        {
            this.CheckDisposed();

            return Ffi.GetHeight(this.h);
        }

        public XmfVpxImageFormat GetFormat()
        {
            this.CheckDisposed();

            return (XmfVpxImageFormat)Ffi.GetFormat(this.h);
        }

        /// <summary>
        /// Returns the start of a plane, or <see cref="IntPtr.Zero"/> when the plane is unavailable.
        /// For I420, the chroma planes have (width + 1) / 2 columns and (height + 1) / 2 rows.
        /// </summary>
        public IntPtr GetPlane(XmfVpxPlane plane)
        {
            this.CheckDisposed();

            return Ffi.GetPlane(this.h, (int)plane);
        }

        /// <summary>
        /// Returns the distance in bytes between rows of a plane, or 0 when the plane is unavailable.
        /// </summary>
        public int GetStride(XmfVpxPlane plane)
        {
            this.CheckDisposed();

            return Ffi.GetStride(this.h, (int)plane);
        }

        /// <summary>
        /// Returns the color space reported by the decoder. VP8 carries no color metadata and reports
        /// <see cref="XmfVpxColorSpace.Unknown"/>.
        /// </summary>
        public XmfVpxColorSpace GetColorSpace()
        {
            this.CheckDisposed();

            return (XmfVpxColorSpace)Ffi.GetColorSpace(this.h);
        }

        /// <summary>
        /// Returns the color range reported by the decoder. VP8 reports <see cref="XmfVpxColorRange.Studio"/>.
        /// </summary>
        public XmfVpxColorRange GetColorRange()
        {
            this.CheckDisposed();

            return (XmfVpxColorRange)Ffi.GetColorRange(this.h);
        }

        private void CheckDisposed()
        {
            if (this.disposed)
            {
                throw new ObjectDisposedException(nameof(XmfVpxImage));
            }
        }
    }
}
