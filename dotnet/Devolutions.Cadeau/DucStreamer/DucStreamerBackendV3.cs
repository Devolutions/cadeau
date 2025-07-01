using System;
using System.Net.WebSockets;
using System.Runtime.InteropServices;
using System.Threading;
using System.Threading.Tasks;

namespace Devolutions.Cadeau
{
    internal class DucStreamerV3Backend : BaseDucStreamerBackend
    {
        internal override uint DucVersion => 3;

        internal override bool KeepAlives => false;

        public override bool IsRawData => true;

        public override bool Connect(Uri destination)
        {
            bool success = true;

            try
            {
                ClientWebSocket webSocket = new ClientWebSocket();
                webSocket.Options.UseDefaultCredentials = false;
                webSocket.ConnectAsync(destination, CancellationToken.None).Wait(this.ConnectTimeout);
                this.Stream = new XmfWsStream(webSocket);
            }
            catch
            {
                success = false;
            }

            return success;
        }

        public override Task Disconnect(CancellationToken token)
        {
            if (this.Stream is XmfWsStream wsStream)
            {
                return wsStream.Close(token);
            }

            return base.Disconnect(token);
        }

        public override bool SendPayload(DateTime timestamp, IntPtr payload, int length)
        {
            if (payload == IntPtr.Zero || length <= 0)
            {
                return false;
            }

            byte[] buffer = new byte[length];
            Marshal.Copy(payload, buffer, 0, length);
            return this.SendRawData(buffer, 0, length);
        }

        public override bool SendPayload(DateTime timestamp, byte[] payload) => this.SendRawData(payload, 0, payload.Length);

        public override Task SendRawDataAsync(byte[] buffer, int offset, int count)
        {
            if (!this.Connected)
            {
                throw new InvalidOperationException("stream is not connected");
            }

            return this.Stream?.WriteAsync(buffer, offset, count);
        }

        public override bool SendRawData(byte[] buffer, int offset, int count)
        {
            if (!this.Connected)
            {
                return false;
            }

            try
            {
                this.Stream?.Write(buffer, offset, count);
            }
            catch (Exception e)
            {
                this.OnError?.Invoke(e);
                return false;
            }

            return true;
        }

#if NETSTANDARD2_1 || NETCOREAPP2_1_OR_GREATER
        public async Task WriteAsync(ReadOnlyMemory<byte> buffer, CancellationToken cancellationToken = default)
        {
            await this.Stream.WriteAsync(buffer, cancellationToken);
        }
#endif
    }
}
