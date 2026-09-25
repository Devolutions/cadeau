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
    /// A decoded frame as returned by <see cref="XmfVpxDecoder.GetNextFrame"/>. Like the native API it reads the
    /// decoder's own buffers without copying, so it is usable only until the next
    /// <see cref="XmfVpxDecoder.Decode(IntPtr, uint)"/> call or until the decoder is disposed; after that,
    /// <see cref="GetPlane"/>, <see cref="GetStride"/> and <see cref="Copy"/> throw. Call <see cref="Copy"/> to keep
    /// the pixels longer. The image keeps the decoder's native memory alive until the image itself is released, so a
    /// stale image never reads freed memory.
    /// </summary>
    public sealed class XmfVpxImage : IDisposable
    {
        private readonly XmfVpxImageHandle h;

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
        /// The pointer is valid only until the next decode call on the decoder, and only while this image is alive:
        /// keep a reference to the image (for example with <see cref="GC.KeepAlive"/>) while reading through it.
        /// </summary>
        public IntPtr GetPlane(XmfVpxPlane plane)
        {
            lock (this.owner.SyncRoot)
            {
                this.CheckUsable();

                return Ffi.GetPlane(this.h, (int)plane);
            }
        }

        /// <summary>
        /// Returns the distance in bytes between rows of a plane, or 0 when the plane is unavailable.
        /// </summary>
        public int GetStride(XmfVpxPlane plane)
        {
            lock (this.owner.SyncRoot)
            {
                this.CheckUsable();

                return Ffi.GetStride(this.h, (int)plane);
            }
        }

        /// <summary>
        /// Copies an 8-bit I420 image into managed memory. The copy stays valid after later decode calls and after the
        /// decoder is disposed. Other formats throw <see cref="NotSupportedException"/>; read them with
        /// <see cref="GetPlane"/> and <see cref="GetStride"/> instead.
        /// </summary>
        public XmfVpxImageCopy Copy()
        {
            lock (this.owner.SyncRoot)
            {
                this.CheckUsable();

                if (this.Format != XmfVpxImageFormat.I420)
                {
                    throw new NotSupportedException($"XmfVpxImage.Copy supports I420 images, not {this.Format}");
                }

                if (this.Width == 0 || this.Height == 0 || this.Width > int.MaxValue / 4 || this.Height > int.MaxValue)
                {
                    throw new InvalidOperationException($"XMF returned an invalid VPX image size {this.Width}x{this.Height}");
                }

                int width = (int)this.Width;
                int height = (int)this.Height;
                int chromaWidth = (width + 1) / 2;
                int chromaHeight = (height + 1) / 2;

                return new XmfVpxImageCopy(
                    this.Width,
                    this.Height,
                    this.Format,
                    this.ColorSpace,
                    this.ColorRange,
                    this.CopyPlane(XmfVpxPlane.Y, width, height),
                    this.CopyPlane(XmfVpxPlane.U, chromaWidth, chromaHeight),
                    this.CopyPlane(XmfVpxPlane.V, chromaWidth, chromaHeight));
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

        private XmfVpxImagePlane CopyPlane(XmfVpxPlane plane, int width, int height)
        {
            IntPtr source = Ffi.GetPlane(this.h, (int)plane);
            int sourceStride = Ffi.GetStride(this.h, (int)plane);
            if (source == IntPtr.Zero || sourceStride < width)
            {
                throw new InvalidOperationException($"XMF returned an incomplete VPX image plane {plane}");
            }

            byte[] data = new byte[checked(width * height)];
            for (int row = 0; row < height; row++)
            {
                Marshal.Copy(IntPtr.Add(source, checked(row * sourceStride)), data, row * width, width);
            }

            return new XmfVpxImagePlane(data, width, height, width);
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

        /// <summary>Bytes per row.</summary>
        public int Stride { get; }
    }

    /// <summary>
    /// An I420 frame copied into managed memory by <see cref="XmfVpxImage.Copy"/>. It does not depend on the decoder.
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
        // The native image points into memory owned by the decoder, so this handle holds a reference on the decoder
        // handle and XmfVpxDecoder_Destroy is deferred until the image is released.
        private XmfVpxDecoderHandle decoder;

        private XmfVpxImageHandle() : base(ownsHandle: true) { }

        internal void KeepDecoderAlive(XmfVpxDecoderHandle owner)
        {
            bool added = false;
            owner.DangerousAddRef(ref added);
            if (added)
            {
                this.decoder = owner;
            }
        }

        protected override bool ReleaseHandle()
        {
            Ffi.Destroy(handle);
            this.decoder?.DangerousRelease();
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
