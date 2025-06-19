using System;
using System.IO;
using System.Threading.Tasks;
using System.Threading;

namespace Devolutions.Cadeau
{
    internal abstract class BaseDucStreamerBackend : IDisposable
    {
        internal abstract uint DucVersion { get; }

        internal abstract bool KeepAlives { get; }

        protected TimeSpan ConnectTimeout { get; set; }

        protected Stream Stream { get; set; }

        public bool Connected => this.Stream?.CanWrite ?? false;

        public abstract bool IsRawData { get; }

        public OnError OnError { get; set; }

        public virtual bool Connect(Uri destination) => false;

        public virtual Task Disconnect(CancellationToken token) => null;

        public virtual bool SendKeepAlive() => true;

        public void Dispose()
        {
            this.Stream?.Dispose();
            this.Stream = null;
        }

        public abstract bool SendPayload(DateTime timestamp, IntPtr payload, int length);

        public abstract bool SendPayload(DateTime timestamp, byte[] payload);

        public abstract Task SendRawDataAsync(byte[] buffer, int offset, int count);

        public abstract bool SendRawData(byte[] buffer, int offset, int count);
    }
}
