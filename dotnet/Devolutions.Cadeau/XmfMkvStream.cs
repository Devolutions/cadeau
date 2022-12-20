using System;
using System.IO;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Devolutions.Cadeau
{
    public class XmfMkvStream : Stream
    {
        public override bool CanRead => true;
        public override bool CanWrite => false;
        public override bool CanSeek => false;

        public XmfBipBuffer bb;

        public XmfMkvStream()
        {
            bb = new XmfBipBuffer();
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
                    IntPtr data = new IntPtr(&ptr[offset]);
                    return bb.Read(data, (nuint) count);
                }
            }
        }
    }
}
