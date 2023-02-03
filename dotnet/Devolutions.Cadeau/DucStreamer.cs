using System;
using System.IO;
using System.IO.Compression;
using System.Net.Security;
using System.Net.Sockets;
using System.Security.Cryptography.X509Certificates;
using System.Text;
using System.Xml.Linq;

namespace Devolutions.Cadeau
{
    public enum DucStreamType : ushort
    {
        Unknown = 0,

        BitmapFrame = 147,

        BitmapUpdate = 148,

        Ssh = 149,

        Wayk = 150
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

        public bool Connect(string host, int port)
        {
            var result = this.ConnectSSL(host, port);
            if (!result)
            {
                this.Disconnect();
                return false;
            }

            result = this.validateServerVersion();
            if (!result)
            {
                this.Disconnect();
                return false;
            }

            return true;
        }

        public void Disconnect()
        {
            if (this.stream == null)
            {
                return;
            }

            if (this.Connected)
            {
                var type = new[] { ENDOFSTREAM_MESSAGE_ID };
                this.stream.Write(type, 0, 1);
            }

            this.stream.Dispose();
        }

        public byte ReceiveServerStatus()
        {
            byte[] status = new byte[1];

            var read = this.stream.Read(status, 0, status.Length);
            return read != status.Length ? (byte)0 : status[0];
        }

        public bool SendClientInfo(string host, DucStreamType type, string authToken)
        {
            const int VERSION_LENGTH = 8;

            if (!this.Connected)
            {
                return false;
            }

            var version = new byte[VERSION_LENGTH];

            try
            {
                this.stream.Write(version, 0, VERSION_LENGTH);
                this.stream.Write(BitConverter.GetBytes((ushort)type), 0, 2);

                this.WriteStringOnStream(host);
                this.WriteStringOnStream(authToken);
            }
            catch (Exception e)
            {
                this.OnError?.Invoke(e);
                return false;
            }

            return true;
        }

        public bool SendKeepAlive()
        {
            if (!this.Connected)
            {
                return false;
            }

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
                uint ts_sec = (uint)timestamp.Subtract(new DateTime(1970, 1, 1)).TotalSeconds;
                uint ts_usec = (uint)(timestamp.Subtract(new DateTime(1970, 1, 1)).TotalMilliseconds - (uint)(timestamp.Subtract(new DateTime(1970, 1, 1)).TotalSeconds * 1000)) * 1000;

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
                uint ts_sec = (uint)timestamp.Subtract(new DateTime(1970, 1, 1)).TotalSeconds;
                uint ts_usec = (uint)(timestamp.Subtract(new DateTime(1970, 1, 1)).TotalMilliseconds - (uint)(timestamp.Subtract(new DateTime(1970, 1, 1)).TotalSeconds * 1000))
                    * 1000;

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

        private bool ConnectSSL(string host, int port)
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

        private bool validateServerVersion()
        {
            byte[] version = new byte[8];

            var read = this.stream.Read(version, 0, 8);
            if (read != 8)
            {
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
    }
}
