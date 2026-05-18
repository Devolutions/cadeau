#if NET10_0_OR_GREATER
#nullable enable

using System;
using System.Net;
using System.Net.Security;
using System.Net.WebSockets;
using System.Threading;
using System.Threading.Tasks;

namespace Devolutions.Cadeau
{
    /// <summary>
    /// Pushes recording payloads to a Devolutions Gateway over a WebSocket (V3 protocol).
    /// </summary>
    /// <remarks>
    /// This client surfaces errors as typed exceptions (notably
    /// <see cref="DucStorageFullException"/>) instead of the older <see cref="DucStreamer"/>
    /// pattern of an <c>OnError</c> callback combined with bool returns. Prefer this client
    /// in new code.
    /// </remarks>
    public sealed class DucPushClient : IAsyncDisposable
    {
        // Private WebSocket close code (4000–4999 range) sent by the gateway when the
        // recording storage volume is full mid-stream. Must match the gateway's
        // STORAGE_FULL_CLOSE_CODE constant.
        private const ushort StorageFullCloseCode = 4010;

        private ClientWebSocket? ws;

        private Task? receiveLoop;
        
        private CancellationTokenSource? receiveLoopCts;

        /// <summary>Timeout for the WebSocket handshake.</summary>
        public TimeSpan ConnectTimeout { get; set; } = TimeSpan.FromSeconds(5);

        /// <summary>Optional HTTP proxy used for the connection.</summary>
        public IWebProxy? Proxy { get; set; }

        /// <summary>Optional server-certificate validation callback.</summary>
        public RemoteCertificateValidationCallback? OnValidateCertificate { get; set; }

        /// <summary>True once <see cref="ConnectAsync"/> succeeds and until the socket closes.</summary>
        public bool Connected => this.ws?.State == WebSocketState.Open;

        /// <summary>
        /// Performs the WebSocket handshake against the gateway's push endpoint.
        /// </summary>
        /// <exception cref="DucStorageFullException">
        /// The gateway refused with HTTP 507 Insufficient Storage (storage full, not writable,
        /// or below the configured free-space threshold).
        /// </exception>
        /// <exception cref="WebSocketException">Other handshake failures.</exception>
        /// <exception cref="OperationCanceledException">Cancellation or timeout.</exception>
        /// <exception cref="InvalidOperationException">Already connected.</exception>
        public async Task ConnectAsync(Uri destination, CancellationToken cancellationToken = default)
        {
            if (this.ws != null)
            {
                throw new InvalidOperationException("DucPushClient is already connected.");
            }

            DucPushLog.Info($"Connecting push stream to {destination}");

            ClientWebSocket socket = this.CreateWebSocket();

            try
            {
                using CancellationTokenSource timeoutCts = new(this.ConnectTimeout);
                using CancellationTokenSource linkedCts =
                    CancellationTokenSource.CreateLinkedTokenSource(cancellationToken, timeoutCts.Token);

                await socket.ConnectAsync(destination, linkedCts.Token).ConfigureAwait(false);
            }
            catch (WebSocketException ex) when (socket.HttpStatusCode == HttpStatusCode.InsufficientStorage)
            {
                socket.Dispose();
                DucPushLog.Warning("Gateway refused push stream: HTTP 507 Insufficient Storage", ex);
                throw new DucStorageFullException(
                    "Gateway refused the recording push: storage is full, not writable, or below the configured free-space threshold.",
                    ex);
            }
            catch
            {
                socket.Dispose();
                throw;
            }

            this.ws = socket;
            this.receiveLoopCts = new CancellationTokenSource();
            this.receiveLoop = this.RunReceiveLoopAsync(this.receiveLoopCts.Token);

            DucPushLog.Debug("Push stream connected");
        }

        /// <summary>
        /// Sends a recording payload frame.
        /// </summary>
        /// <exception cref="DucStorageFullException">
        /// The gateway closed the stream with close code <c>4010</c> (storage full mid-stream).
        /// </exception>
        /// <exception cref="WebSocketException">Other transport errors.</exception>
        /// <exception cref="InvalidOperationException">Not connected.</exception>
        public async Task SendAsync(ReadOnlyMemory<byte> payload, CancellationToken cancellationToken = default)
        {
            ClientWebSocket socket = this.ws ?? throw new InvalidOperationException("DucPushClient is not connected.");

            try
            {
                await socket.SendAsync(payload, WebSocketMessageType.Binary, endOfMessage: true, cancellationToken)
                    .ConfigureAwait(false);
            }
            catch (Exception ex) when (IsStorageFullClose(socket))
            {
                DucPushLog.Warning("Gateway closed push stream: storage full (close code 4010)", ex);
                throw new DucStorageFullException(
                    "Gateway closed the recording push stream: storage is full.",
                    ex);
            }
        }

        /// <summary>
        /// Sends a normal-closure close frame and waits for the receive loop to terminate.
        /// </summary>
        /// <exception cref="OperationCanceledException">
        /// <paramref name="cancellationToken"/> was cancelled before the close handshake completed.
        /// The receive loop is still stopped before this propagates.
        /// </exception>
        public async Task CloseAsync(CancellationToken cancellationToken = default)
        {
            ClientWebSocket? socket = this.ws;
            if (socket == null)
            {
                return;
            }

            try
            {
                if (socket.State == WebSocketState.Open || socket.State == WebSocketState.CloseReceived)
                {
                    try
                    {
                        await socket.CloseAsync(WebSocketCloseStatus.NormalClosure, statusDescription: string.Empty, cancellationToken)
                            .ConfigureAwait(false);
                    }
                    catch (OperationCanceledException)
                    {
                        // The caller cancelled; surface it (the receive loop is still stopped in finally).
                        throw;
                    }
                    catch (Exception ex)
                    {
                        // Best-effort close. The peer may have already dropped the connection.
                        DucPushLog.Debug($"Push-stream close handshake failed: {ex.Message}");
                    }
                }
            }
            finally
            {
                await this.StopReceiveLoopAsync().ConfigureAwait(false);
            }
        }

        public async ValueTask DisposeAsync()
        {
            ClientWebSocket? socket = this.ws;
            this.ws = null;

            if (socket == null)
            {
                return;
            }

            try
            {
                if (socket.State == WebSocketState.Open)
                {
                    using CancellationTokenSource cts = new(TimeSpan.FromSeconds(1));
                    await socket.CloseAsync(WebSocketCloseStatus.NormalClosure, statusDescription: string.Empty, cts.Token)
                        .ConfigureAwait(false);
                }
            }
            catch (Exception ex)
            {
                // Swallow — we're disposing.
                DucPushLog.Debug($"Push-stream close during dispose failed: {ex.Message}");
            }

            await this.StopReceiveLoopAsync().ConfigureAwait(false);
            socket.Dispose();
        }

        /// <summary>
        /// Background loop that drains incoming frames so the WebSocket state machine processes
        /// close frames from the server (which is what populates <see cref="WebSocket.CloseStatus"/>).
        /// The push protocol is otherwise one-way: the server is not expected to send payload data.
        /// </summary>
        private async Task RunReceiveLoopAsync(CancellationToken cancellationToken)
        {
            ClientWebSocket socket = this.ws!;
            byte[] buffer = new byte[256];

            try
            {
                while (!cancellationToken.IsCancellationRequested)
                {
                    WebSocketReceiveResult result = await socket.ReceiveAsync(new ArraySegment<byte>(buffer), cancellationToken)
                        .ConfigureAwait(false);

                    if (result.MessageType == WebSocketMessageType.Close)
                    {
                        // The server initiated a close. CloseStatus is now populated; SendAsync
                        // callers will surface DucStorageFullException if appropriate.
                        DucPushLog.Debug(
                            $"Server initiated push-stream close: code={(ushort?)socket.CloseStatus}, description=\"{socket.CloseStatusDescription}\"");
                        return;
                    }

                    // Push protocol does not expect server-to-client data; discard anything received.
                }
            }
            catch (OperationCanceledException)
            {
                // Normal shutdown via our own cancellation token.
                DucPushLog.Trace("Push-stream receive loop cancelled");
            }
            catch (Exception ex)
            {
                // Receive loop is best-effort; on error, exit and let SendAsync surface the issue.
                DucPushLog.Debug($"Push-stream receive loop ended on error: {ex.Message}");
            }
        }

        private async Task StopReceiveLoopAsync()
        {
            CancellationTokenSource? cts = this.receiveLoopCts;
            Task? loop = this.receiveLoop;
            this.receiveLoopCts = null;
            this.receiveLoop = null;

            cts?.Cancel();
            if (loop != null)
            {
                try
                {
                    await loop.ConfigureAwait(false);
                }
                catch (Exception ex)
                {
                    // The receive loop is designed to swallow its own errors; this should be
                    // unreachable. Log at Trace if it ever isn't, so we can investigate.
                    DucPushLog.Trace($"Push-stream receive loop awaited with exception: {ex.Message}");
                }
            }
            cts?.Dispose();
        }

        private static bool IsStorageFullClose(ClientWebSocket socket)
        {
            return socket.CloseStatus is { } status && (ushort)status == StorageFullCloseCode;
        }

        private ClientWebSocket CreateWebSocket()
        {
            ClientWebSocket socket = new();
            socket.Options.UseDefaultCredentials = false;
            // Required for ClientWebSocket.HttpStatusCode to be populated on a failed handshake
            // (so we can detect HTTP 507 from the gateway pre-flight rejection).
            socket.Options.CollectHttpResponseDetails = true;

            if (this.Proxy != null)
            {
                socket.Options.Proxy = this.Proxy;
            }

            if (this.OnValidateCertificate != null)
            {
                socket.Options.RemoteCertificateValidationCallback += this.OnValidateCertificate;
            }

            return socket;
        }
    }
}

#endif
