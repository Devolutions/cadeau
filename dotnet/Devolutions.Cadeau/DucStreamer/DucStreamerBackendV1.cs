using System;
using System.Collections.Generic;
using System.IO;
using System.IO.Compression;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace Devolutions.Cadeau
{
    internal class DucStreamerV1Backend : BaseDucStreamerBackend
    {
        protected const byte ENDOFSTREAM_MESSAGE_ID = 3;

        protected const byte KEEPALIVE_MESSAGE_ID = 4;

        protected const byte METADATA_MESSAGE_ID = 1;

        protected const byte PAYLOAD_MESSAGE_ID = 2;

        internal override uint DucVersion => 1;

        internal override bool KeepAlives => true;

        public override bool IsRawData => false;

        public DucStreamerMetadata Metadata { get; set; }

        internal DucStreamerV1Backend(Stream stream)
        {
            this.Stream = stream;
        }

        internal virtual bool SendClientHello()
        {
            if (!this.Connected)
            {
                return false;
            }

            try
            {
                this.Stream?.Write(BitConverter.GetBytes(this.DucVersion), 0, 4);
                this.Stream?.Write(BitConverter.GetBytes((uint)0), 0, 4);
                this.Stream?.Write(BitConverter.GetBytes((ushort)this.Metadata.StreamType), 0, 2);

                this.WriteStringOnStream(this.Metadata.TargetHost);
                this.WriteStringOnStream(this.Metadata.AuthToken);
            }
            catch (Exception e)
            {
                this.OnError?.Invoke(e);
                return false;
            }

            return true;
        }

        public override Task Disconnect(CancellationToken token)
        {
            byte[] type = { ENDOFSTREAM_MESSAGE_ID };
            this.Stream?.Write(type, 0, 1);

            return base.Disconnect(token);
        }

        public override bool SendKeepAlive()
        {
            if (!this.Connected)
            {
                return false;
            }
            byte[] type = { KEEPALIVE_MESSAGE_ID };

            try
            {
                this.Stream?.Write(type, 0, 1);
            }
            catch (Exception e)
            {
                this.OnError?.Invoke(e);
                return false;
            }

            return true;
        }

        internal byte ReceiveServerStatus()
        {
            byte[] status = new byte[1];
            int? read = this.Stream?.Read(status, 0, status.Length);
            return read != status.Length ? (byte)0 : status[0];
        }

        internal bool SendMetadata(string name, string value)
        {
            if (!this.Connected)
            {
                return false;
            }

            byte[] type = { METADATA_MESSAGE_ID };

            try
            {
                this.Stream?.Write(type, 0, 1);

                this.WriteStringOnStream(name);
                this.WriteStringOnStream(value);
            }
            catch (Exception e)
            {
                this.OnError?.Invoke(e);
                return false;
            }

            return true;
        }

        public override unsafe bool SendPayload(DateTime timestamp, IntPtr payload, int length)
        {
            if (!this.Connected)
            {
                return false;
            }

            byte[] type = { PAYLOAD_MESSAGE_ID };
            byte[] record = new byte[16];

            try
            {
                (uint tsSec, uint tsUsec) = this.GetPcapTimestamp(timestamp);

                this.Stream?.Write(type, 0, 1);

                using MemoryStream ms = new MemoryStream();
                using (UnmanagedMemoryStream ums = new UnmanagedMemoryStream((byte*)payload.ToPointer(), length))
                {
                    using GZipStream zip = new GZipStream(ms, CompressionLevel.Fastest);
                    ums.CopyTo(zip);
                }

                // FIXME: GetBuffer returns the full buffer which is probably overallocated. Use ms.Length instead of zippedPayload.Length.
                byte[] zippedPayload = ms.GetBuffer();

                tsSec.CopyToByteArray(record, 0);
                tsUsec.CopyToByteArray(record, 4);
                zippedPayload.Length.CopyToByteArray(record, 8);
                length.CopyToByteArray(record, 12);

                this.Stream?.Write(BitConverter.GetBytes(record.Length + zippedPayload.Length), 0, 4);
                this.Stream?.Write(record, 0, record.Length);
                this.Stream?.Write(zippedPayload, 0, zippedPayload.Length);
            }
            catch (Exception e)
            {
                this.OnError?.Invoke(e);
                return false;
            }

            return true;
        }

        public override bool SendPayload(DateTime timestamp, byte[] payload)
        {
            if (!this.Connected)
            {
                return false;
            }

            byte[] type = { PAYLOAD_MESSAGE_ID };
            byte[] record = new byte[16];

            try
            {
                (uint tsSec, uint tsUsec) = this.GetPcapTimestamp(timestamp);

                this.Stream?.Write(type, 0, 1);

                using MemoryStream ms = new MemoryStream();
                using (GZipStream zip = new GZipStream(ms, CompressionLevel.Fastest, true))
                {
                    zip.Write(payload, 0, payload.Length);
                }

                // FIXME: GetBuffer returns the full buffer which is probably overallocated. Use ms.Length instead of zippedPayload.Length.
                byte[] zippedPayload = ms.GetBuffer();

                tsSec.CopyToByteArray(record, 0);
                tsUsec.CopyToByteArray(record, 4);
                zippedPayload.Length.CopyToByteArray(record, 8);
                payload.Length.CopyToByteArray(record, 12);

                this.Stream?.Write(BitConverter.GetBytes(record.Length + zippedPayload.Length), 0, 4);
                this.Stream?.Write(record, 0, record.Length);
                this.Stream?.Write(zippedPayload, 0, zippedPayload.Length);
            }
            catch (Exception e)
            {
                this.OnError?.Invoke(e);
                return false;
            }

            return true;
        }

#if NETSTANDARD2_1 || NETCOREAPP2_1_OR_GREATER
        public async Task WriteAsync(DateTime timestamp, byte[] payload, int offset, int count, CancellationToken cancellationToken = default)
        {
            using MemoryStream ms = new MemoryStream(payload, offset, count);
            await this.WriteAsync(timestamp, ms, cancellationToken);
        }

        public unsafe Task WriteAsync(DateTime timestamp, IntPtr payload, int length, CancellationToken cancellationToken = default)
        {
            using UnmanagedMemoryStream ms = new UnmanagedMemoryStream((byte*)payload.ToPointer(), length);
            return this.WriteAsync(timestamp, ms, cancellationToken);
        }

        private async Task WriteAsync(DateTime timestamp, Stream payload, CancellationToken cancellationToken = default)
        {
            byte[] type = { PAYLOAD_MESSAGE_ID };
            byte[] record = new byte[16];

            try
            {
                (uint tsSec, uint tsUsec) = this.GetPcapTimestamp(timestamp);

                await this.Stream.WriteAsync(type, 0, 1, cancellationToken);

                using MemoryStream ms = new MemoryStream();
                await using (GZipStream zip = new GZipStream(ms, CompressionLevel.Fastest, true))
                {
                    await zip.CopyToAsync(payload, cancellationToken);
                }

                byte[] zippedPayload = ms.GetBuffer();

                tsSec.CopyToByteArray(record, 0);
                tsUsec.CopyToByteArray(record, 4);
                zippedPayload.Length.CopyToByteArray(record, 8);
                ((int)payload.Length).CopyToByteArray(record, 12);

                await this.Stream.WriteAsync(BitConverter.GetBytes(record.Length + zippedPayload.Length), 0, 4, cancellationToken);
                await this.Stream.WriteAsync(record, 0, record.Length, cancellationToken);
                await this.Stream.WriteAsync(zippedPayload, 0, zippedPayload.Length, cancellationToken);
            }
            catch (Exception e)
            {
                this.OnError?.Invoke(e);
                throw;
            }
        }
#endif

        public override Task SendRawDataAsync(byte[] buffer, int offset, int count) => Task.Run(() => this.SendRawData(buffer, offset, count));

        public override bool SendRawData(byte[] buffer, int offset, int count) => this.SendPayload(DateTime.Now, buffer.Skip(offset).Take(count).ToArray());

        protected (uint ts_sec, uint ts_usec) GetPcapTimestamp(DateTime timestamp)
        {
            DateTime unixEpoch = new DateTime(1970, 1, 1, 0, 0, 0, DateTimeKind.Utc);

            uint tsSec = (uint)timestamp.Subtract(unixEpoch).TotalSeconds;
            uint tsUsec = (uint)(timestamp.Subtract(unixEpoch).TotalMilliseconds -
                                 (uint)(timestamp.Subtract(unixEpoch).TotalSeconds * 1000)) * 1000;

            return (tsSec, tsUsec);
        }

        protected void WriteStringOnStream(string source)
        {
            byte[] bytes = Encoding.UTF8.GetBytes(source);
            this.Stream?.Write(BitConverter.GetBytes(bytes.Length), 0, 4);
            this.Stream?.Write(bytes, 0, bytes.Length);
        }

        internal sealed class DucStreamerMetadata
        {
            internal Dictionary<string, string> AdditionalMetadata { get; set; }

            internal string AuthToken { get; set; }

            internal string FileType { get; set; }

            internal uint FrameHeight { get; set; }

            internal uint FrameRate { get; set; }

            internal uint FrameWidth { get; set; }

            internal DucStreamType StreamType { get; set; }

            internal string TargetHost { get; set; }
        }
    }
}
