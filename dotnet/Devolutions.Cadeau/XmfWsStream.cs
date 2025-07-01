using System;
using System.IO;
using System.Net.WebSockets;
using System.Threading;
using System.Threading.Tasks;

namespace Devolutions.Cadeau
{
    public class XmfWsStream : Stream
    {
        public override bool CanRead => false;
        public override bool CanWrite => true;
        public override bool CanSeek => false;

        private ClientWebSocket ws;

        public XmfWsStream(ClientWebSocket webSocket)
        {
            this.ws = webSocket;
        }

        public override long Length => throw new NotImplementedException();

        public override long Position
        {
            get => throw new NotImplementedException();
            set => throw new NotImplementedException();
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
            this.WriteAsync(buffer, offset, count, CancellationToken.None).Wait();
        }

        public override Task WriteAsync(byte[] buffer, int offset, int count, CancellationToken cancellationToken)
        {
            ArraySegment<byte> data = new ArraySegment<byte>(buffer, offset, count);
            return this.ws.SendAsync(data, WebSocketMessageType.Binary, true, cancellationToken);
        }

#if NETSTANDARD2_1 || NETCOREAPP2_1_OR_GREATER
        public override ValueTask WriteAsync(ReadOnlyMemory<byte> buffer, CancellationToken cancellationToken = default)
        {
            return this.ws.SendAsync(buffer, WebSocketMessageType.Binary, true, cancellationToken);
        }
#endif

        public override void Flush()
        {
            // do nothing
        }

        public override int Read(byte[] buffer, int offset, int count)
        {
            throw new NotImplementedException();
        }

        public Task Close(CancellationToken cancellationToken)
        {
            return this.ws.CloseAsync(WebSocketCloseStatus.NormalClosure, string.Empty, cancellationToken);
        }
    }
}
