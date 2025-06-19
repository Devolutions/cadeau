namespace Devolutions.Cadeau
{
    public class DucStreamerInfo
    {
        public uint DucVersion { get; private set; }

        public bool KeepAlives { get; private set; }

        public bool RawDataStream { get; private set; }

        internal DucStreamerInfo(BaseDucStreamerBackend streamer)
        {
            this.DucVersion = streamer?.DucVersion ?? 0;
            this.KeepAlives = streamer?.KeepAlives ?? false;
            this.RawDataStream = streamer?.IsRawData ?? false;
        }
    }
}
