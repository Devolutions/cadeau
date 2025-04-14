using System;
using System.Collections.Generic;
using System.IO;
using System.IO.Compression;
using System.Net.Security;
using System.Net.Sockets;
using System.Net.WebSockets;
using System.Security.Cryptography.X509Certificates;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace Devolutions.Cadeau
{
    public enum DucStreamType : ushort
    {
        Unknown = 0,

        BitmapFrame = 147,

        BitmapUpdate = 148, // deprecated

        Ssh = 149,

        Wayk = 150, // deprecated

        Raw = 175 // raw file bytes
    }

    public delegate void OnError(Exception e);

    public delegate bool OnValidateCertificate(object sender, X509Certificate certificate, X509Chain chain, SslPolicyErrors sslPolicyErrors);

    public class DucStreamer
    {
        private const byte ENDOFSTREAM_MESSAGE_ID = 3;

        private const byte KEEPALIVE_MESSAGE_ID = 4;

        private const byte METADATA_MESSAGE_ID = 1;

        private const byte PAYLOAD_MESSAGE_ID = 2;

        private const int RecordLength = 16;

        private readonly TimeSpan connectTimeout = TimeSpan.FromSeconds(5);

        private Stream stream;

        private uint ducVersion = 2;

        public bool Connected => this.stream?.CanWrite ?? false;

        public OnError OnError { get; set; }

        public OnValidateCertificate OnValidateCertificate { get; set; }

        public DucStreamType streamType;

        public string FileType = "webm";

        public string TargetHost;

        public string AuthToken;

        public Dictionary<string, string> metadata = new();

        public bool IsRawData => this.ducVersion >= 2;

        public uint FrameWidth { get; set; } = 1024;

        public uint FrameHeight { get; set; } = 768;

        public uint FrameRate { get; set; } = 5;

        public bool ConnectOld(string scheme, string host, int port)
        {
            if (!(scheme == "tcp" ? this.ConnectTcp(host, port) : this.ConnectTls(host, port)))
            {
                this.Disconnect(CancellationToken.None);
                return false;
            }

            if (!this.Handshake())
            {
                this.Disconnect(CancellationToken.None);
                return false;
            }

            return true;
        }

        public bool ConnectV3(string destinationUrl)
        {
            Uri url = new(destinationUrl);

            try
            {
                ClientWebSocket webSocket = new();
                webSocket.Options.UseDefaultCredentials = false;

                using CancellationTokenSource cts = new(this.connectTimeout);
                webSocket.ConnectAsync(url, cts.Token);
                this.stream = new XmfWsStream(webSocket);

                return true;
            }
            catch
            {
                return false;
            }
        }

        public bool ConnectUrl(string destinationUrl)
        {
            if (destinationUrl.IndexOf("://") < 0)
            {
                destinationUrl = "tls://" + destinationUrl;
            }

            if (destinationUrl.StartsWith("ws"))
            {
                this.ducVersion = 3;
                return this.ConnectV3(destinationUrl);
            }

            Uri url = new(destinationUrl);
            return this.ConnectOld(url.Scheme, url.Host, url.Port);
        }

        public bool Handshake()
        {
            if (!this.RecvServerHello())
            {
                return false;
            }

            if (this.ducVersion == 2)
            {
                if (!this.SendClientHelloV2())
                {
                    return false;
                }
            }
            else
            {
                if (!this.SendClientHelloV1())
                {
                    return false;
                }

                if (this.ReceiveServerStatus() != 1)
                {
                    return false;
                }

                foreach (KeyValuePair<string, string> elem in this.metadata)
                {
                    this.SendMetadata(elem.Key, elem.Value);
                }
            }

            return true;
        }

        public Task Disconnect(CancellationToken token)
        {
            try
            {
                if (this.stream == null)
                {
                    return null;
                }

                if (this.Connected && !this.IsRawData)
                {
                    byte[] type = { ENDOFSTREAM_MESSAGE_ID };
                    this.stream.Write(type, 0, 1);
                }

                if (this.stream is XmfWsStream wsStream)
                {
                    return wsStream.Close(token);
                }

                return null;
            }
            finally
            {

                this.stream?.Dispose();
            }
        }

        public bool SendClientHelloV2()
        {
            if (!this.Connected)
            {
                return false;
            }

            StringBuilder sb = new();

            sb.Append("TargetHost: " + this.TargetHost + "\n");
            sb.Append("AuthToken: " + this.AuthToken + "\n");

            sb.Append("FileType: " + this.FileType + "\n");
            sb.Append("FrameWidth: " + this.FrameWidth + "\n");
            sb.Append("FrameHeight: " + this.FrameHeight + "\n");
            sb.Append("FrameRate: " + this.FrameRate + "\n");

            foreach (KeyValuePair<string, string> elem in this.metadata)
            {
                sb.Append(elem.Key + ": " + elem.Value + "\n");
            }

            string clientInfo = sb.ToString();

            byte[] data = Encoding.UTF8.GetBytes(clientInfo);

            try
            {
                this.stream.Write(BitConverter.GetBytes(this.ducVersion), 0, 4);
                this.stream.Write(BitConverter.GetBytes(data.Length), 0, 4);
                this.stream.Write(data, 0, data.Length);
            }
            catch (Exception e)
            {
                this.OnError?.Invoke(e);
                return false;
            }

            return true;
        }

        public bool SendClientHelloV1()
        {
            if (!this.Connected)
            {
                return false;
            }

            this.metadata.Add("Width", this.FrameWidth.ToString());
            this.metadata.Add("Height", this.FrameHeight.ToString());
            this.metadata.Add("FPS", this.FrameRate.ToString());

            try
            {
                this.stream.Write(BitConverter.GetBytes(this.ducVersion), 0, 4);
                this.stream.Write(BitConverter.GetBytes((uint)0), 0, 4);

                this.stream.Write(BitConverter.GetBytes((ushort)this.streamType), 0, 2);
                this.WriteStringOnStream(this.TargetHost);
                this.WriteStringOnStream(this.AuthToken);
            }
            catch (Exception e)
            {
                this.OnError?.Invoke(e);
                return false;
            }

            return true;
        }

        public byte ReceiveServerStatus()
        {
            byte[] status = new byte[1];
            int read = this.stream.Read(status, 0, status.Length);
            return read != status.Length ? (byte)0 : status[0];
        }

        public bool SendKeepAlive()
        {
            if (!this.Connected)
            {
                return false;
            }

            if (!this.IsRawData)
            {
                byte[] type = { KEEPALIVE_MESSAGE_ID };

                try
                {
                    this.stream.Write(type, 0, 1);
                }
                catch (Exception e)
                {
                    this.OnError?.Invoke(e);
                    return false;
                }
            }

            return true;
        }

        public bool SendMetadata(string name, string value)
        {
            if (!this.Connected)
            {
                return false;
            }

            byte[] type = { METADATA_MESSAGE_ID };

            try
            {
                this.stream.Write(type, 0, 1);

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

        public bool SendRawData(byte[] buffer, int offset, int count)
        {
            if (!this.Connected)
            {
                return false;
            }

            if (!this.IsRawData)
            {
                return false;
            }

            try
            {
                this.stream.Write(buffer, offset, count);
            }
            catch (Exception e)
            {
                this.OnError?.Invoke(e);
                return false;
            }

            return true;
        }

        public async Task<bool> SendRawDataAsync(byte[] buffer, int offset, int count, CancellationToken cancellationToken)
        {
            if (!this.Connected)
            {
                return false;
            }

            if (!this.IsRawData)
            {
                return false;
            }

            try
            {
                await this.stream.WriteAsync(buffer, offset, count, cancellationToken);
            }
            catch (Exception e)
            {
                this.OnError?.Invoke(e);
                return false;
            }

            return true;
        }

        private (uint ts_sec, uint ts_usec) GetPcapTimestamp(DateTime timestamp)
        {
            DateTime unixEpoch = new(1970, 1, 1, 0, 0, 0, DateTimeKind.Utc);
            uint ts_sec = (uint)timestamp.Subtract(unixEpoch).TotalSeconds;
            uint ts_usec = (uint)(timestamp.Subtract(unixEpoch).TotalMilliseconds -
                (uint)(timestamp.Subtract(unixEpoch).TotalSeconds * 1000)) * 1000;
            return (ts_sec, ts_usec);
        }

        public bool SendPayload(DateTime timestamp, byte[] payload)
        {
            return this.SendPayload(timestamp, payload.AsSpan());
        }

        public unsafe bool SendPayload(DateTime timestamp, IntPtr payload, int length)
        {
            return this.SendPayload(timestamp, new ReadOnlySpan<byte>(payload.ToPointer(), length));
        }

        private bool SendPayload(DateTime timestamp, ReadOnlySpan<byte> payload)
        {
            if (!this.Connected)
            {
                return false;
            }

            try
            {
                (uint ts_sec, uint ts_usec) = this.GetPcapTimestamp(timestamp);

                using MemoryStream ms = new();
                using GZipStream zip = new(ms, CompressionLevel.Fastest);
                zip.Write(payload);

                byte[] zippedPayload = ms.GetBuffer();

                using BinaryWriter writer = new(this.stream);
                // Header
                writer.Write(PAYLOAD_MESSAGE_ID);
                writer.Write(BitConverter.GetBytes(RecordLength + zippedPayload.Length));
                // Record fields
                writer.Write(ts_sec);
                writer.Write(ts_usec);
                writer.Write(zippedPayload.Length);
                writer.Write(payload.Length);
                // Payload
                this.stream.Write(zippedPayload, 0, zippedPayload.Length);
            }
            catch (Exception e)
            {
                this.OnError?.Invoke(e);
                return false;
            }

            return true;
        }

        private void WriteStringOnStream(string source)
        {
            using BinaryWriter writer = new(this.stream, Encoding.UTF8, true);
            writer.Write(Encoding.UTF8.GetByteCount(source));
            writer.Write(source);
        }

        private bool ConnectTcp(string host, int port)
        {
            try
            {
                TcpClient client = new();
                client.Connect(host, port);

                this.stream = client.GetStream();
            }
            catch (Exception e)
            {
                this.OnError?.Invoke(e);
                return false;
            }

            return true;
        }

        private bool ConnectTls(string host, int port)
        {
            try
            {
                TcpClient client = new();
                client.Connect(host, port);

                NetworkStream clientStream = client.GetStream();

                RemoteCertificateValidationCallback certificateValidationCallback = null;

                if (this.OnValidateCertificate != null)
                {
                    certificateValidationCallback = this.ValidateServerCertificate;
                }

                SslStream sslStream = new(clientStream, false, certificateValidationCallback);
                sslStream.AuthenticateAsClient(host);
                this.stream = sslStream;
            }
            catch (Exception e)
            {
                this.OnError?.Invoke(e);
                return false;
            }

            return true;
        }

        private bool ValidateServerCertificate(object sender, X509Certificate certificate, X509Chain chain, SslPolicyErrors sslPolicyErrors) => this.OnValidateCertificate?.Invoke(sender, certificate, chain, sslPolicyErrors) ?? false;

        private bool RecvServerHello()
        {
            byte[] hello = new byte[8];

            int read = this.stream.Read(hello, 0, 8);

            if (read != 8)
            {
                return false;
            }

            unsafe
            {
                fixed (byte* ptr = hello)
                {
                    uint ducVersion = *((uint*)&ptr[0]);
                    //uint ducReserved = *((uint*)&ptr[4]);

                    if (ducVersion >= 2 && this.ducVersion >= 2)
                    {
                        this.ducVersion = 2;
                    }
                    else
                    {
                        this.ducVersion = 1;
                    }
                }
            }

            return true;
        }
    }
}
