using Microsoft.Win32.SafeHandles;
using System;
using System.Runtime.InteropServices;

namespace Devolutions.Cadeau
{
    public enum XmfVpxCodec
    {
        VP8 = 0,
        VP9 = 1,
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct XmfVpxDecoderConfig
    {
        /// <summary>Maximum number of decoding threads; 0 or 1 decodes on one thread.</summary>
        public uint Threads;

        /// <summary>Width, or 0 when unknown.</summary>
        public uint Width;

        /// <summary>Height, or 0 when unknown.</summary>
        public uint Height;

        public XmfVpxCodec Codec;
    }

    public enum XmfVpxDecoderErrorCode
    {
        NoError = 0,
        MemoryError = 1,
        InitError = 2,
        DecodeError = 3,
        NoFrameAvailable = 4,
        VpxError = 5,
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct XmfVpxDecoderError
    {
        public XmfVpxDecoderErrorCode Code;

        /// <summary>The libvpx <c>vpx_codec_err_t</c> when <see cref="Code"/> is <see cref="XmfVpxDecoderErrorCode.VpxError"/>.</summary>
        public int VpxErrorCode;
    }

    public sealed class XmfVpxDecoderHandle : SafeHandleZeroOrMinusOneIsInvalid
    {
        private XmfVpxDecoderHandle() : base(ownsHandle: true) { }

        protected override bool ReleaseHandle()
        {
            Ffi.Destroy(handle);
            return true;
        }

        internal static class Ffi
        {
            private const string Lib = "xmf";

            [DllImport(Lib, EntryPoint = "XmfVpxDecoder_Create")]
            internal static extern XmfVpxDecoderHandle Create(XmfVpxDecoderConfig config);

            [DllImport(Lib, EntryPoint = "XmfVpxDecoder_Destroy")]
            internal static extern void Destroy(IntPtr decoder);
        }
    }

    public class XmfVpxDecoder : IDisposable
    {
        private readonly XmfVpxDecoderHandle h;

        private bool disposed;

        private class Ffi
        {
            private const string Lib = "xmf";

            [DllImport(Lib, EntryPoint = "XmfVpxDecoder_Decode")]
            public static extern int Decode(XmfVpxDecoderHandle decoder, IntPtr data, uint size);

            [DllImport(Lib, EntryPoint = "XmfVpxDecoder_GetNextFrame")]
            public static extern XmfVpxImageHandle GetNextFrame(XmfVpxDecoderHandle decoder);

            [DllImport(Lib, EntryPoint = "XmfVpxDecoder_GetLastError")]
            public static extern XmfVpxDecoderError GetLastError(XmfVpxDecoderHandle decoder);
        }

        public XmfVpxDecoderHandle Handle => this.h;

        public XmfVpxDecoder(XmfVpxDecoderConfig config)
        {
            this.h = XmfVpxDecoderHandle.Ffi.Create(config) ?? throw new InvalidOperationException("XmfVpxDecoder_Create failed");

            if (this.h.IsInvalid)
            {
                throw new InvalidOperationException("XmfVpxDecoder_Create failed");
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

        /// <summary>
        /// Decodes one compressed frame. Returns false on failure; see <see cref="GetLastError"/>.
        /// </summary>
        public bool Decode(IntPtr data, uint size)
        {
            this.CheckDisposed();

            return Ffi.Decode(this.h, data, size) == 0;
        }

        public bool Decode(byte[] data)
        {
            if (data == null)
            {
                throw new ArgumentNullException(nameof(data));
            }

            GCHandle pinned = GCHandle.Alloc(data, GCHandleType.Pinned);
            try
            {
                return this.Decode(pinned.AddrOfPinnedObject(), (uint)data.Length);
            }
            finally
            {
                pinned.Free();
            }
        }

        /// <summary>
        /// Returns a copy of the next decoded frame, or null when none is left. The copy does not depend on this
        /// decoder, so it stays valid after later decode calls and after the decoder is disposed.
        /// </summary>
        public XmfVpxImage GetNextFrame()
        {
            this.CheckDisposed();

            // The frame's planes live in the decoder's buffers, so keep the decoder alive while copying them out.
            bool addedReference = false;
            this.h.DangerousAddRef(ref addedReference);
            try
            {
                using (XmfVpxImageHandle image = Ffi.GetNextFrame(this.h))
                {
                    if (image == null || image.IsInvalid)
                    {
                        return null;
                    }

                    return XmfVpxImage.CopyFrom(image);
                }
            }
            finally
            {
                if (addedReference)
                {
                    this.h.DangerousRelease();
                }
            }
        }

        public XmfVpxDecoderError GetLastError()
        {
            this.CheckDisposed();

            return Ffi.GetLastError(this.h);
        }

        private void CheckDisposed()
        {
            if (this.disposed)
            {
                throw new ObjectDisposedException(nameof(XmfVpxDecoder));
            }
        }
    }
}
