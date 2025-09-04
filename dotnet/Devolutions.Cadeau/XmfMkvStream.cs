using System;
using System.IO;

namespace Devolutions.Cadeau
{
    public class XmfMkvStream : Stream
    {
        public override bool CanRead => true;

        public override bool CanWrite => false;

        public override bool CanSeek => false;

        [Obsolete("Use BipBuffer property instead")]
        public XmfBipBuffer bb => this.BipBuffer;

        private XmfBipBuffer bipBuffer = new();

        public XmfBipBuffer BipBuffer
        {
            get => this.bipBuffer;
            private set => this.bipBuffer = value;
        }

        public XmfMkvStream()
        {
        }

        protected override void Dispose(bool disposing)
        {
            this.BipBuffer?.Dispose();
            this.BipBuffer = null;

            base.Dispose(disposing);
        }

        public override long Length
        {
            get { throw new NotImplementedException(); }
        }

        public override long Position
        {
            get { throw new NotImplementedException(); }
            set { throw new NotImplementedException(); }
        }

        public override long Seek(long offset, SeekOrigin origin)
        {
            throw new NotImplementedException();
        }

        public override void SetLength(long value)
        {
            throw new NotImplementedException();
        }

        public override void Write(byte[] buffer, int offset, int count)
        {
            throw new NotImplementedException();
        }

        public override void Flush()
        {
            // do nothing
        }

        public override int Read(byte[] buffer, int offset, int count)
        {
            unsafe {
                fixed (byte* ptr = buffer) {
                    IntPtr data = new(&ptr[offset]);
                    return this.BipBuffer.Read(data, (nuint) count);
                }
            }
        }
    }
}
