using System;
using System.Collections.Generic;
using System.IO;
using System.Text;

namespace Devolutions.Cadeau
{
    internal class DucStreamerV2Backend : DucStreamerV1Backend
    {
        internal override uint DucVersion => 2;

        public override bool IsRawData => false;

        internal DucStreamerV2Backend(Stream stream) : base(stream)
        {
        }

        internal override bool SendClientHello()
        {
            if (!this.Connected)
            {
                return false;
            }

            StringBuilder sb = new StringBuilder();

            sb.Append("TargetHost: " + this.Metadata.TargetHost + "\n");
            sb.Append("AuthToken: " + this.Metadata.AuthToken + "\n");

            sb.Append("FileType: " + this.Metadata.FileType + "\n");
            sb.Append("FrameWidth: " + this.Metadata.FrameWidth + "\n");
            sb.Append("FrameHeight: " + this.Metadata.FrameHeight + "\n");
            sb.Append("FrameRate: " + this.Metadata.FrameRate + "\n");

            foreach (KeyValuePair<string, string> elem in this.Metadata.AdditionalMetadata)
            {
                sb.Append(elem.Key + ": " + elem.Value + "\n");
            }

            string clientInfo = sb.ToString();

            byte[] data = Encoding.UTF8.GetBytes(clientInfo);

            try
            {
                this.Stream?.Write(BitConverter.GetBytes(this.DucVersion), 0, 4);
                this.Stream?.Write(BitConverter.GetBytes(data.Length), 0, 4);
                this.Stream?.Write(data, 0, data.Length);
            }
            catch (Exception e)
            {
                this.OnError?.Invoke(e);
                return false;
            }

            return true;
        }
    }
}
