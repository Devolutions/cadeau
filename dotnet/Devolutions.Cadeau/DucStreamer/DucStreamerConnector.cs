using System;
using System.Collections.Generic;
using System.IO;
using System.Net.Security;
using System.Net.Sockets;

namespace Devolutions.Cadeau
{
    internal class DucStreamerConnector
    {
        internal OnError OnError { get; set; }

        internal OnValidateCertificate OnValidateCertificate { get; set; }

        internal bool TryHandshake(Stream stream, DucStreamerV1Backend.DucStreamerMetadata metadata, out BaseDucStreamerBackend streamerBackend)
        {
            streamerBackend = null;

            try
            {
                uint ducVersion = this.RecvServerHello(stream);

                switch (ducVersion)
                {
                    case 1:
                    {
                        DucStreamerV1Backend s = new DucStreamerV1Backend(stream);

                        if (!s.SendClientHello())
                        {
                            return false;
                        }

                        if (s.ReceiveServerStatus() != 1)
                        {
                            return false;
                        }

                        metadata.AdditionalMetadata["Width"] = metadata.FrameWidth.ToString();
                        metadata.AdditionalMetadata["Height"] = metadata.FrameHeight.ToString();
                        metadata.AdditionalMetadata["FPS"] = metadata.FrameRate.ToString();

                        foreach (KeyValuePair<string, string> elem in metadata.AdditionalMetadata)
                        {
                            if (!s.SendMetadata(elem.Key, elem.Value))
                            {
                                return false;
                            }
                        }

                        streamerBackend = s;
                        return true;
                    }

                    case 2:
                    {
                        DucStreamerV2Backend s = new DucStreamerV2Backend(stream);

                        if (!s.SendClientHello())
                        {
                            return false;
                        }

                        streamerBackend = s;
                        return true;
                    }

                    default:
                    {
                        streamerBackend = null;
                        return false;
                    }
                }

            }
            catch (Exception e)
            {
                this.OnError?.Invoke(e);

                streamerBackend = null;
                return false;
            }
        }

        internal Stream ConnectTcp(string host, int port)
        {
            TcpClient client = new TcpClient();
            client.Connect(host, port);

            return client.GetStream();
        }

        internal Stream ConnectTls(string host, int port)
        {
            TcpClient client = new TcpClient();
            client.Connect(host, port);

            NetworkStream clientStream = client.GetStream();
            RemoteCertificateValidationCallback certificateValidationCallback = null;

            if (this.OnValidateCertificate == null)
            {
                certificateValidationCallback = (_, _, _, _) => false;
            }

            SslStream sslStream = new SslStream(clientStream, false, certificateValidationCallback);
            sslStream.AuthenticateAsClient(host);
            return sslStream;
        }

        private uint RecvServerHello(Stream stream)
        {
            byte[] hello = new byte[8];
            int? read = stream?.Read(hello, 0, 8);

            if (read.GetValueOrDefault() != 8)
            {
                return 0;
            }

            uint ducVersion = BitConverter.ToUInt32(hello, 0);

            return (uint) (ducVersion >= 2 ? 2 : 1);
        }
    }
}
