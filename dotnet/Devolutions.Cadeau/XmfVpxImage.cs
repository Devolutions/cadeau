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

    /// <summary>
    /// One plane of a decoded image. Rows are tightly packed: row r starts at r * <see cref="Stride"/>.
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
    /// A decoded frame copied out of the decoder, so it stays valid after later decode calls and after the
    /// decoder is disposed.
    /// </summary>
    public sealed class XmfVpxImage
    {
        private XmfVpxImage(
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

        /// <summary>
        /// Color space reported by the decoder. VP8 carries no color metadata and reports
        /// <see cref="XmfVpxColorSpace.Unknown"/>.
        /// </summary>
        public XmfVpxColorSpace ColorSpace { get; }

        /// <summary>Color range reported by the decoder. VP8 reports <see cref="XmfVpxColorRange.Studio"/>.</summary>
        public XmfVpxColorRange ColorRange { get; }

        public XmfVpxImagePlane Y { get; }

        public XmfVpxImagePlane U { get; }

        public XmfVpxImagePlane V { get; }

        // The caller keeps the owning decoder alive while this runs, because the planes live in its buffers.
        internal static XmfVpxImage CopyFrom(XmfVpxImageHandle image)
        {
            uint width = Ffi.GetWidth(image);
            uint height = Ffi.GetHeight(image);
            XmfVpxImageFormat format = (XmfVpxImageFormat)Ffi.GetFormat(image);
            if (!TryGetLayout(format, out int xShift, out int yShift, out int bytesPerSample))
            {
                throw new NotSupportedException($"XMF returned an unsupported VPX image format {format}");
            }

            if (width == 0 || height == 0 || width > int.MaxValue / 4 || height > int.MaxValue)
            {
                throw new InvalidOperationException($"XMF returned an invalid VPX image size {width}x{height}");
            }

            int lumaWidth = (int)width;
            int lumaHeight = (int)height;
            int chromaWidth = (lumaWidth + (1 << xShift) - 1) >> xShift;
            int chromaHeight = (lumaHeight + (1 << yShift) - 1) >> yShift;

            return new XmfVpxImage(
                width,
                height,
                format,
                (XmfVpxColorSpace)Ffi.GetColorSpace(image),
                (XmfVpxColorRange)Ffi.GetColorRange(image),
                CopyPlane(image, 0, lumaWidth, lumaHeight, bytesPerSample),
                CopyPlane(image, 1, chromaWidth, chromaHeight, bytesPerSample),
                CopyPlane(image, 2, chromaWidth, chromaHeight, bytesPerSample));
        }

        private static XmfVpxImagePlane CopyPlane(XmfVpxImageHandle image, int plane, int width, int height, int bytesPerSample)
        {
            IntPtr source = Ffi.GetPlane(image, plane);
            int sourceStride = Ffi.GetStride(image, plane);
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
