#if NETSTANDARD2_1 || NETCOREAPP2_1_OR_GREATER
using System;
using System.Buffers;

namespace Devolutions.Cadeau
{
    internal sealed class UnmanagedMemoryManager : MemoryManager<byte>
    {
        private readonly IntPtr pointer;

        private readonly int length;

        public UnmanagedMemoryManager(IntPtr pointer, int length)
        {
            this.pointer = pointer;
            this.length = length;
        }

        public override unsafe Span<byte> GetSpan() => new Span<byte>(this.pointer.ToPointer(), this.length);

        public override unsafe MemoryHandle Pin(int elementIndex = 0)
        {
            return new MemoryHandle((byte*) this.pointer + elementIndex);
        }

        public override void Unpin()
        {
        }

        protected override void Dispose(bool disposing)
        {
        }
    }
}
#endif
