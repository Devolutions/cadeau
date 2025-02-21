using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.IO.Compression;
using System.Net.Security;
using System.Net.Sockets;
using System.Net.WebSockets;
using System.Runtime.InteropServices;
using System.Security.Cryptography.X509Certificates;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Xml.Linq;

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

    internal static class Extensions
    {
        public static void CopyToByteArray(this int source, byte[] destination, int offset)
        {
            if (destination == null)
            {
                return;
            }

            if (destination.Length < offset + 4)
            {
                return;
            }

            destination[offset] = (byte)source;
            destination[offset + 1] = (byte)(source >> 8);
            destination[offset + 2] = (byte)(source >> 16);
            destination[offset + 3] = (byte)(source >> 24);
        }

        public static void CopyToByteArray(this uint source, byte[] destination, int offset)
        {
            if (destination == null)
            {
                return;
            }

            if (destination.Length < offset + 4)
            {
                return;
            }

            destination[offset] = (byte)source;
            destination[offset + 1] = (byte)(source >> 8);
            destination[offset + 2] = (byte)(source >> 16);
            destination[offset + 3] = (byte)(source >> 24);
        }
    }

    public delegate void OnError(Exception e);

    public delegate bool OnValidateCertificate(X509Certificate certificate, X509Chain chain, SslPolicyErrors sslPolicyErrors);

    public class DucStreamer
    {
        private const byte ENDOFSTREAM_MESSAGE_ID = 3;

        private const byte KEEPALIVE_MESSAGE_ID = 4;

        private const byte METADATA_MESSAGE_ID = 1;

        private const byte PAYLOAD_MESSAGE_ID = 2;

        private Stream stream;

        public bool Connected => this.stream?.CanWrite ?? false;

        public OnError OnError { get; set; }

        public OnValidateCertificate OnValidateCertificate { get; set; }

        public DucStreamType streamType;

        public string FileType = "webm";

        public string TargetHost;

        public string AuthToken;

        public Dictionary<string, string> metadata = new Dictionary<string, string>();

        private uint DucVersion = 2;

        private int connectTimeout = 5000;

        public bool IsRawData { get { return this.DucVersion >= 2; } }

        public uint FrameWidth = 1024;

        public uint FrameHeight = 768;

        public uint FrameRate = 5;

        public bool ConnectOld(string scheme, string host, int port)
        {
            bool result = false;

            if (scheme == "tcp")
            {
                result = this.ConnectTcp(host, port);
            }
            else
            {
                result = this.ConnectTls(host, port);
            }

            if (!result)
            {
                this.Disconnect(null);
                return false;
            }

            result = this.Handshake();

            if (!result)
            {
                this.Disconnect(null);
                return false;
            }

            return result;
        }

        public bool ConnectV3(string destinationUrl)
        {
            bool success = true;
            Uri url = new Uri(destinationUrl);

            try
            {
                ClientWebSocket webSocket = new ClientWebSocket();
                webSocket.Options.UseDefaultCredentials = false;
                webSocket.ConnectAsync(url, CancellationToken.None).Wait(this.connectTimeout);
                this.stream = new XmfWsStream(webSocket);
            }
            catch
            {
                success = false;
            }

            return success;
        }

        public bool ConnectUrl(string destinationUrl)
        {
            if (destinationUrl.IndexOf("://") < 0)
            {
                destinationUrl = "tls://" + destinationUrl;
            }

            if (destinationUrl.StartsWith("ws"))
            {
                this.DucVersion = 3;
                return this.ConnectV3(destinationUrl);
            }

            Uri url = new Uri(destinationUrl);
            return this.ConnectOld(url.Scheme, url.Host, url.Port);
        }

        public bool Handshake()
        {
            if (!this.RecvServerHello())
            {
                return false;
            }

            if (this.DucVersion == 2)
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

                int status = this.ReceiveServerStatus();

                if (status != 1)
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

        public Task? Disconnect(CancellationToken? token)
        {
            try
            {

                if (this.stream == null)
                {
                    return null;
                }

                if (this.Connected && !this.IsRawData)
                {
                    var type = new[] { ENDOFSTREAM_MESSAGE_ID };
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

                this.stream.Dispose();
            }
        }

        public bool SendClientHelloV2()
        {
            if (!this.Connected)
            {
                return false;
            }

            StringBuilder sb = new StringBuilder();

            sb.Append("TargetHost: " + this.TargetHost + "\n");
            sb.Append("AuthToken: " + this.AuthToken + "\n");

            sb.Append("FileType: " + this.FileType + "\n");
            sb.Append("FrameWidth: " + this.FrameWidth.ToString() + "\n");
            sb.Append("FrameHeight: " + this.FrameHeight.ToString() + "\n");
            sb.Append("FrameRate: " + this.FrameRate.ToString() + "\n");

            foreach (KeyValuePair<string, string> elem in this.metadata)
            {
                sb.Append(elem.Key + ": " + elem.Value + "\n");
            }

            string clientInfo = sb.ToString();

            byte[] data = Encoding.UTF8.GetBytes(clientInfo);

            try
            {
                this.stream.Write(BitConverter.GetBytes(this.DucVersion), 0, 4);
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
                this.stream.Write(BitConverter.GetBytes(this.DucVersion), 0, 4);
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
            var read = this.stream.Read(status, 0, status.Length);
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
                var type = new[] { KEEPALIVE_MESSAGE_ID };

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

            var type = new[] { METADATA_MESSAGE_ID };

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

        public unsafe bool SendRawData(byte[] buffer, int offset, int count)
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

        (uint ts_sec, uint ts_usec) GetPcapTimestamp(DateTime timestamp)
        {
            var unixEpoch = new DateTime(1970, 1, 1, 0, 0, 0, DateTimeKind.Utc);
            uint ts_sec = (uint)timestamp.Subtract(unixEpoch).TotalSeconds;
            uint ts_usec = (uint)(timestamp.Subtract(unixEpoch).TotalMilliseconds -
                (uint)(timestamp.Subtract(unixEpoch).TotalSeconds * 1000)) * 1000;
            return (ts_sec, ts_usec);
        }

        public unsafe bool SendPayload(DateTime timestamp, IntPtr payload, int length)
        {
            if (!this.Connected)
            {
                return false;
            }

            var type = new[] { PAYLOAD_MESSAGE_ID };
            var record = new byte[16];

            try
            {
                (uint ts_sec, uint ts_usec) = this.GetPcapTimestamp(timestamp);

                this.stream.Write(type, 0, 1);

                using (var ms = new MemoryStream())
                {
                    using (var ums = new UnmanagedMemoryStream((byte*)payload.ToPointer(), length))
                    {
                        using (var zip = new GZipStream(ms, CompressionLevel.Fastest))
                        {
                            ums.CopyTo(zip);
                        }
                    }

                    var zippedPayload = ms.GetBuffer();

                    ts_sec.CopyToByteArray(record, 0);
                    ts_usec.CopyToByteArray(record, 4);
                    zippedPayload.Length.CopyToByteArray(record, 8);
                    length.CopyToByteArray(record, 12);

                    this.stream.Write(BitConverter.GetBytes(record.Length + zippedPayload.Length), 0, 4);
                    this.stream.Write(record, 0, record.Length);
                    this.stream.Write(zippedPayload, 0, zippedPayload.Length);
                }
            }
            catch (Exception e)
            {
                this.OnError?.Invoke(e);
                return false;
            }

            return true;
        }

        public bool SendPayload(DateTime timestamp, byte[] payload)
        {
            if (!this.Connected)
            {
                return false;
            }

            var type = new[] { PAYLOAD_MESSAGE_ID };
            var record = new byte[16];

            try
            {
                (uint ts_sec, uint ts_usec) = this.GetPcapTimestamp(timestamp);

                this.stream.Write(type, 0, 1);

                using (var ms = new MemoryStream())
                {
                    using (var zip = new GZipStream(ms, CompressionLevel.Fastest))
                    {
                        zip.Write(payload, 0, payload.Length);
                    }

                    var zippedPayload = ms.GetBuffer();

                    ts_sec.CopyToByteArray(record, 0);
                    ts_usec.CopyToByteArray(record, 4);
                    zippedPayload.Length.CopyToByteArray(record, 8);
                    payload.Length.CopyToByteArray(record, 12);

                    this.stream.Write(BitConverter.GetBytes(record.Length + zippedPayload.Length), 0, 4);
                    this.stream.Write(record, 0, record.Length);
                    this.stream.Write(zippedPayload, 0, zippedPayload.Length);
                }
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
            var bytes = Encoding.UTF8.GetBytes(source);
            this.stream.Write(BitConverter.GetBytes(bytes.Length), 0, 4);
            this.stream.Write(bytes, 0, bytes.Length);
        }

        private bool ConnectTcp(string host, int port)
        {
            try
            {
                var client = new TcpClient();
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
                var client = new TcpClient();
                client.Connect(host, port);

                var clientStream = client.GetStream();

                RemoteCertificateValidationCallback certificateValidationCallback = null;

                if (this.OnValidateCertificate != null)
                {
                    certificateValidationCallback = this.ValidateServerCertificate;
                }

                SslStream sslStream = new SslStream(clientStream, false, certificateValidationCallback);
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

        private bool ValidateServerCertificate(object sender, X509Certificate certificate, X509Chain chain, SslPolicyErrors sslPolicyErrors)
        {
            if (this.OnValidateCertificate == null)
            {
                return false;
            }

            return this.OnValidateCertificate(certificate, chain, sslPolicyErrors);
        }

        private bool RecvServerHello()
        {
            byte[] hello = new byte[8];

            var read = this.stream.Read(hello, 0, 8);

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

                    if ((ducVersion >= 2) && (this.DucVersion >= 2))
                    {
                        this.DucVersion = 2;
                    }
                    else
                    {
                        this.DucVersion = 1;
                    }
                }
            }

            return true;
        }
    }
}
