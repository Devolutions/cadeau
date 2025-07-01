using System;
using System.Collections.Generic;
using System.IO;
using System.Net.Security;
using System.Security.Cryptography.X509Certificates;
using System.Threading;
using System.Threading.Tasks;

namespace Devolutions.Cadeau
{
    public delegate void OnError(Exception e);

    public delegate bool OnValidateCertificate(object sender, X509Certificate certificate, X509Chain chain, SslPolicyErrors sslPolicyErrors);

    public class DucStreamer
    {
        private BaseDucStreamerBackend streamer;

        private BaseDucStreamerBackend Streamer
        {
            get => this.streamer;
            set
            {
                this.streamer = value;

                if (this.streamer != null)
                {
                    this.streamer.OnError = this.OnError;

#if NETSTANDARD2_1 || NETCOREAPP2_1_OR_GREATER
                    this.writeAdaptor = new WriteAdaptor(this.streamer);
#endif
                }
            }
        }

#if NETSTANDARD2_1 || NETCOREAPP2_1_OR_GREATER
        private WriteAdaptor writeAdaptor;
#endif

        [Obsolete("Use StreamType instead.")] public DucStreamType streamType;

        [Obsolete("Use Metadata instead.")] public Dictionary<string, string> metadata = new();

        public string AuthToken { get; set; }

        public bool Connected => this.Streamer?.Connected ?? false;

        public DucStreamerInfo StreamerInfo => new(this.Streamer);

        public string FileType { get; set; } = "webm";

        public uint FrameHeight { get; set; } = 768;

        public uint FrameRate { get; set; } = 5;

        public uint FrameWidth { get; set; } = 1024;

        public bool IsRawData => this.Streamer?.IsRawData ?? false;

        public Dictionary<string, string> Metadata
        {
            get => this.metadata;
            set => this.metadata = value;
        }

        public OnError OnError { get; set; }

        public OnValidateCertificate OnValidateCertificate { get; set; }

        public DucStreamType StreamType
        {
            get => this.streamType;
            set => this.streamType = value;
        }

        public string TargetHost { get; set; }

        public bool ConnectUrl(string destination)
        {
            if (destination?.IndexOf("://") < 0)
            {
                destination = "tls://" + destination;
            }

            if (!Uri.TryCreate(destination, UriKind.Absolute, out Uri destinationUri))
            {
                return false;
            }

            return ConnectUrl(destinationUri);
        }

        public bool ConnectUrl(Uri destination)
        {
            if (IsV3Address(destination))
            {
                this.Streamer = new DucStreamerV3Backend();
                return this.Streamer.Connect(destination);
            }

            return this.ConnectOld(destination.Scheme, destination.Host, destination.Port);
        }

        public static bool IsV3Address(Uri destination) => destination.Scheme.StartsWith("ws");

        public static bool IsV3Address(string destination) => Uri.TryCreate(destination, UriKind.Absolute, out Uri uri) && IsV3Address(uri);

        public Task Disconnect(CancellationToken token)
        {
            try
            {

                if (!this.Streamer?.Connected ?? false)
                {
                    return null;
                }

                return this.Streamer?.Disconnect(token);
            }
            finally
            {

                this.Streamer?.Dispose();
                this.Streamer = null;
            }
        }

        public bool SendKeepAlive() => this.Streamer?.SendKeepAlive() ?? true;

        public bool SendPayload(DateTime timestamp, IntPtr payload, int length) => this.Streamer?.SendPayload(timestamp, payload, length) ?? false;

        public bool SendPayload(DateTime timestamp, byte[] payload) => this.Streamer?.SendPayload(timestamp, payload) ?? false;

        public Task SendRawDataAsync(byte[] buffer, int offset, int count) => this.Streamer?.SendRawDataAsync(buffer, offset, count) ?? Task.CompletedTask;

        public bool SendRawData(byte[] buffer, int offset, int count) => this.Streamer?.SendRawData(buffer, offset, count) ?? false;

#if NETSTANDARD2_1 || NETCOREAPP2_1_OR_GREATER
        public Task WriteAsync(byte[] buffer, int offset, int count, CancellationToken cancellationToken = default) => this.writeAdaptor?.WriteAsync(buffer, offset, count, cancellationToken);

        public Task WriteAsync(IntPtr buffer, int length, CancellationToken cancellationToken = default) => this.writeAdaptor?.WriteAsync(buffer, length, cancellationToken);
#endif

        private bool ConnectOld(string scheme, string host, int port)
        {
            DucStreamerConnector connector = new DucStreamerConnector()
            {
                OnValidateCertificate = this.OnValidateCertificate
            };

            Stream stream = null;

            try
            {
                stream = scheme == "tcp" ? connector.ConnectTcp(host, port) : connector.ConnectTls(host, port);

                DucStreamerV1Backend.DucStreamerMetadata md = new DucStreamerV1Backend.DucStreamerMetadata
                {
                    AuthToken = this.AuthToken,
                    FileType = this.FileType,
                    FrameHeight = this.FrameHeight,
                    FrameRate = this.FrameRate,
                    FrameWidth = this.FrameWidth,
                    AdditionalMetadata = this.Metadata,
                    TargetHost = this.TargetHost,
                };

                if (connector.TryHandshake(stream, md, out BaseDucStreamerBackend s))
                {
                    this.Streamer = s;
                    return true;
                }

                throw new Exception("handshake failed");

            }
            catch (Exception e)
            {
                this.Disconnect(CancellationToken.None);
                stream?.Dispose();

                this.OnError?.Invoke(e);

                return false;
            }
        }
    }

#if NETSTANDARD2_1 || NETCOREAPP2_1_OR_GREATER
    internal interface IWriteAdaptor
    {
        Task WriteAsync(byte[] buffer, int offset, int count, CancellationToken cancellationToken = default);

        Task WriteAsync(IntPtr buffer, int length, CancellationToken cancellationToken = default);
    }

    internal class WriteAdaptor : IWriteAdaptor
    {
        private readonly Func<byte[], int, int, CancellationToken, Task> writeAsync;

        private readonly Func<IntPtr, int, CancellationToken, Task> writeIntPtrAsync;

        public Task WriteAsync(byte[] buffer, int offset, int count, CancellationToken cancellationToken = default) => writeAsync(buffer, offset, count, cancellationToken);

        public Task WriteAsync(IntPtr buffer, int length, CancellationToken cancellationToken = default) => writeIntPtrAsync(buffer, length, cancellationToken);

        public WriteAdaptor(BaseDucStreamerBackend streamer)
        {
            switch (streamer)
            {
                case DucStreamerV1Backend legacy:
                {
                    writeAsync = (buffer, offset, count, cancellationToken) =>
                        legacy.WriteAsync(DateTime.Now, buffer, offset, count, cancellationToken);

                    writeIntPtrAsync = (ptr, length, cancellationToken) =>
                        legacy.WriteAsync(DateTime.Now, ptr, length, cancellationToken);

                    break;
                }

                case DucStreamerV3Backend v3:
                {
                    writeAsync = (buffer, offset, count, cancellationToken) =>
                        v3.WriteAsync(new ReadOnlyMemory<byte>(buffer, offset, count), cancellationToken);

                    writeIntPtrAsync = (buffer, length, cancellationToken) =>
                    {
                        using UnmanagedMemoryManager umm = new UnmanagedMemoryManager(buffer, length);
                        return v3.WriteAsync(umm.Memory, cancellationToken);
                    };

                    break;
                }

                default:
                    throw new NotImplementedException("unsupported streamer type");
            }
        }
    }
#endif
}
