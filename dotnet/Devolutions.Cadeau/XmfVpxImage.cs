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

    /// <summary>
    /// A decoded frame as returned by <see cref="XmfVpxDecoder.GetNextFrame"/>. Like the native API, it reads the
    /// decoder's own buffers without copying, so it is usable only until the next <see cref="XmfVpxDecoder.Decode(IntPtr, uint)"/>
    /// call or until the decoder is disposed; after that, <see cref="GetPlane"/>, <see cref="GetStride"/> and
    /// <see cref="Copy"/> throw. Call <see cref="Copy"/> to keep the pixels longer.
    /// </summary>
    public sealed class XmfVpxImage : IDisposable
    {
        private readonly XmfVpxImageHandle h;

        // Holding the decoder keeps it from being finalized while this view can still read its buffers.
        private readonly XmfVpxDecoder owner;

        private readonly int generation;

        private bool disposed;

        internal XmfVpxImage(XmfVpxImageHandle h, XmfVpxDecoder owner, int generation)
        {
            this.h = h;
            this.owner = owner;
            this.generation = generation;
            this.Width = Ffi.GetWidth(h);
            this.Height = Ffi.GetHeight(h);
            this.Format = (XmfVpxImageFormat)Ffi.GetFormat(h);
            this.ColorSpace = (XmfVpxColorSpace)Ffi.GetColorSpace(h);
            this.ColorRange = (XmfVpxColorRange)Ffi.GetColorRange(h);
        }

        public uint Width { get; }

        public uint Height { get; }

        public XmfVpxImageFormat Format { get; }

        /// <summary>
        /// Color space reported by the decoder. VP8 carries no color metadata and reports
        /// <see cref="XmfVpxColorSpace.Unknown"/>.
        /// </summary>
        public XmfVpxColorSpace ColorSpace { get; }

        /// <summary>Color range reported by the decoder. VP8 reports <see cref="XmfVpxColorRange.Studio"/>.</summary>
        public XmfVpxColorRange ColorRange { get; }

        /// <summary>
        /// Returns the start of a plane in decoder memory, or <see cref="IntPtr.Zero"/> when the plane is unavailable.
        /// The pointer must not be used after the next decode call or after the decoder is disposed.
        /// </summary>
        public IntPtr GetPlane(XmfVpxPlane plane)
        {
            this.CheckUsable();

            return Ffi.GetPlane(this.h, (int)plane);
        }

        /// <summary>
        /// Returns the distance in bytes between rows of a plane, or 0 when the plane is unavailable.
        /// </summary>
        public int GetStride(XmfVpxPlane plane)
        {
            this.CheckUsable();

            return Ffi.GetStride(this.h, (int)plane);
        }

        /// <summary>
        /// Copies the Y, U and V planes into managed memory. The copy stays valid after later decode calls and after
        /// the decoder is disposed. Supports 8- and 16-bit I420, YV12, I422, I440 and I444.
        /// </summary>
        public XmfVpxImageCopy Copy()
        {
            this.CheckUsable();

            if (!TryGetLayout(this.Format, out int xShift, out int yShift, out int bytesPerSample))
            {
                throw new NotSupportedException($"XMF returned an unsupported VPX image format {this.Format}");
            }

            if (this.Width == 0 || this.Height == 0 || this.Width > int.MaxValue / 4 || this.Height > int.MaxValue)
            {
                throw new InvalidOperationException($"XMF returned an invalid VPX image size {this.Width}x{this.Height}");
            }

            int lumaWidth = (int)this.Width;
            int lumaHeight = (int)this.Height;
            int chromaWidth = (lumaWidth + (1 << xShift) - 1) >> xShift;
            int chromaHeight = (lumaHeight + (1 << yShift) - 1) >> yShift;

            bool addedReference = false;
            this.owner.Handle.DangerousAddRef(ref addedReference);
            try
            {
                return new XmfVpxImageCopy(
                    this.Width,
                    this.Height,
                    this.Format,
                    this.ColorSpace,
                    this.ColorRange,
                    this.CopyPlane(XmfVpxPlane.Y, lumaWidth, lumaHeight, bytesPerSample),
                    this.CopyPlane(XmfVpxPlane.U, chromaWidth, chromaHeight, bytesPerSample),
                    this.CopyPlane(XmfVpxPlane.V, chromaWidth, chromaHeight, bytesPerSample));
            }
            finally
            {
                if (addedReference)
                {
                    this.owner.Handle.DangerousRelease();
                }
            }
        }

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

        private XmfVpxImagePlane CopyPlane(XmfVpxPlane plane, int width, int height, int bytesPerSample)
        {
            IntPtr source = Ffi.GetPlane(this.h, (int)plane);
            int sourceStride = Ffi.GetStride(this.h, (int)plane);
            int rowBytes = checked(width * bytesPerSample);
            if (source == IntPtr.Zero || sourceStride < rowBytes)
            {
                throw new InvalidOperationException($"XMF returned an incomplete VPX image plane {plane}");
            }

            byte[] data = new byte[checked(rowBytes * height)];
            for (int row = 0; row < height; row++)
            {
                Marshal.Copy(IntPtr.Add(source, checked(row * sourceStride)), data, row * rowBytes, rowBytes);
            }

            return new XmfVpxImagePlane(data, width, height, rowBytes);
        }

        private void CheckUsable()
        {
            if (this.disposed)
            {
                throw new ObjectDisposedException(nameof(XmfVpxImage));
            }

            if (!this.owner.IsCurrentGeneration(this.generation))
            {
                throw new InvalidOperationException(
                    "The decoded image is no longer valid: its decoder has decoded again or was disposed. Copy it first to keep the pixels.");
            }
        }

        private static bool TryGetLayout(XmfVpxImageFormat format, out int xShift, out int yShift, out int bytesPerSample)
        {
            bytesPerSample = ((int)format & 0x800) != 0 ? 2 : 1;
            switch ((XmfVpxImageFormat)((int)format & ~0x800))
            {
                case XmfVpxImageFormat.I420:
                case XmfVpxImageFormat.Yv12:
                    xShift = 1;
                    yShift = 1;
                    return true;
                case XmfVpxImageFormat.I422:
                    xShift = 1;
                    yShift = 0;
                    return true;
                case XmfVpxImageFormat.I440:
                    xShift = 0;
                    yShift = 1;
                    return true;
                case XmfVpxImageFormat.I444:
                    xShift = 0;
                    yShift = 0;
                    return true;
                default:
                    xShift = 0;
                    yShift = 0;
                    return false;
            }
        }

        private static class Ffi
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
    }

    /// <summary>
    /// One plane of an <see cref="XmfVpxImageCopy"/>. Rows are tightly packed: row r starts at r * <see cref="Stride"/>.
    /// </summary>
    public sealed class XmfVpxImagePlane
    {
        internal XmfVpxImagePlane(byte[] data, int width, int height, int stride)
        {
            this.Data = data;
            this.Width = width;
            this.Height = height;
            this.Stride = stride;
        }

        public byte[] Data { get; }

        /// <summary>Samples per row.</summary>
        public int Width { get; }

        /// <summary>Number of rows.</summary>
        public int Height { get; }

        /// <summary>Bytes per row: Width for 8-bit formats, Width * 2 for 16-bit formats.</summary>
        public int Stride { get; }
    }

    /// <summary>
    /// A decoded frame copied into managed memory by <see cref="XmfVpxImage.Copy"/>. It does not depend on the decoder.
    /// </summary>
    public sealed class XmfVpxImageCopy
    {
        internal XmfVpxImageCopy(
            uint width,
            uint height,
            XmfVpxImageFormat format,
            XmfVpxColorSpace colorSpace,
            XmfVpxColorRange colorRange,
            XmfVpxImagePlane y,
            XmfVpxImagePlane u,
            XmfVpxImagePlane v)
        {
            this.Width = width;
            this.Height = height;
            this.Format = format;
            this.ColorSpace = colorSpace;
            this.ColorRange = colorRange;
            this.Y = y;
            this.U = u;
            this.V = v;
        }

        public uint Width { get; }

        public uint Height { get; }

        public XmfVpxImageFormat Format { get; }

        public XmfVpxColorSpace ColorSpace { get; }

        public XmfVpxColorRange ColorRange { get; }

        public XmfVpxImagePlane Y { get; }

        public XmfVpxImagePlane U { get; }

        public XmfVpxImagePlane V { get; }
    }

    internal sealed class XmfVpxImageHandle : SafeHandleZeroOrMinusOneIsInvalid
    {
        private XmfVpxImageHandle() : base(ownsHandle: true) { }

        protected override bool ReleaseHandle()
        {
            Ffi.Destroy(handle);
            return true;
        }

        private static class Ffi
        {
            private const string Lib = "xmf";

            [DllImport(Lib, EntryPoint = "XmfVpxImage_Destroy")]
            public static extern void Destroy(IntPtr image);
        }
    }
}
