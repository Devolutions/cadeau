using Microsoft.Win32.SafeHandles;
using System;
using System.Runtime.InteropServices;
using System.Threading;

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
        private int disposeRequested;

        private XmfVpxDecoderHandle() : base(ownsHandle: true) { }

        internal bool IsDisposeRequested => Volatile.Read(ref this.disposeRequested) != 0;

        protected override void Dispose(bool disposing)
        {
            if (disposing)
            {
                Volatile.Write(ref this.disposeRequested, 1);
            }

            base.Dispose(disposing);
        }

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

        // Serializes decoding with reads of the decoder's buffers through XmfVpxImage.
        internal readonly object SyncRoot = new object();

        private int generation;

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

            lock (this.SyncRoot)
            {
                this.disposed = true;
            }

            this.h?.Dispose();
            GC.SuppressFinalize(this);
        }

        /// <summary>
        /// Decodes one compressed frame. Returns false on failure; see <see cref="GetLastError"/>. Images returned
        /// earlier become unusable, because the decoder reuses their buffers.
        /// </summary>
        public bool Decode(IntPtr data, uint size)
        {
            lock (this.SyncRoot)
            {
                this.CheckDisposed();

                this.generation++;
                return Ffi.Decode(this.h, data, size) == 0;
            }
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
        /// Returns the next decoded frame without copying it, or null when none is left. The image reads this
        /// decoder's buffers and is usable only until the next <see cref="Decode(IntPtr, uint)"/> call or until the
        /// decoder is disposed; call <see cref="XmfVpxImage.Copy"/> to keep the pixels longer.
        /// </summary>
        public XmfVpxImage GetNextFrame()
        {
            lock (this.SyncRoot)
            {
                this.CheckDisposed();

                XmfVpxImageHandle image = Ffi.GetNextFrame(this.h);
                if (image == null || image.IsInvalid)
                {
                    image?.Dispose();
                    return null;
                }

                try
                {
                    image.KeepDecoderAlive(this.h);
                    return new XmfVpxImage(image, this, this.generation);
                }
                catch
                {
                    image.Dispose();
                    throw;
                }
            }
        }

        // Callers hold SyncRoot. Disposing Handle directly also invalidates images: they keep its native memory alive,
        // so the handle is only released, and IsClosed only set, once they are gone.
        internal bool IsCurrentGeneration(int imageGeneration)
        {
            return !this.disposed && !this.h.IsDisposeRequested && imageGeneration == this.generation;
        }

        public XmfVpxDecoderError GetLastError()
        {
            lock (this.SyncRoot)
            {
                this.CheckDisposed();

                return Ffi.GetLastError(this.h);
            }
        }

        // Handle can be disposed directly, bypassing Dispose; images may still keep its native memory alive, but the
        // decoder must stop working as soon as either one is disposed.
        private void CheckDisposed()
        {
            if (this.disposed || this.h.IsDisposeRequested)
            {
                throw new ObjectDisposedException(nameof(XmfVpxDecoder));
            }
        }
    }
}
