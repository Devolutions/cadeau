namespace Devolutions.Cadeau
{
    internal static class GatewayPushCloseCodes
    {
        // Private WebSocket close code (4000-4999 range) sent by a producer to tell the gateway
        // the recording is over. The gateway then ends the recording at once instead of keeping
        // it open for a reconnect (any other close, such as 1000, keeps it open for the token's
        // reconnection window). Must match the gateway's RECORDING_ENDED_CLOSE_CODE constant.
        internal const ushort RecordingEnded = 4011;
    }
}
