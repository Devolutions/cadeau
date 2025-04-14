using System;
using System.IO;
using System.Net.WebSockets;
using System.Threading;
using System.Threading.Tasks;

namespace Devolutions.Cadeau
{
    public class XmfWsStream : Stream
    {
        private readonly ClientWebSocket ws;

        public override bool CanRead => false;

        public override bool CanWrite => true;

        public override bool CanSeek => false;

        internal XmfWsStream(ClientWebSocket webSocket)
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
            ArraySegment<byte> data = new ArraySegment<byte>(buffer, offset, count);
            this.ws.SendAsync(data, WebSocketMessageType.Binary, true, CancellationToken.None).Wait();
        }

        public override async Task WriteAsync(byte[] buffer, int offset, int count, CancellationToken cancellationToken)
        {
            ArraySegment<byte> data = new ArraySegment<byte>(buffer, offset, count);
            await this.ws.SendAsync(data, WebSocketMessageType.Binary, true, cancellationToken);
        }

        public override void Flush()
        {
            // do nothing
        }

        public override int Read(byte[] buffer, int offset, int count)
        {
            throw new NotImplementedException();
        }

        public Task Close(CancellationToken? cancellationToken = null)
        {
            return this.ws.CloseAsync(WebSocketCloseStatus.NormalClosure, string.Empty, cancellationToken.GetValueOrDefault(CancellationToken.None));
        }
    }
}
