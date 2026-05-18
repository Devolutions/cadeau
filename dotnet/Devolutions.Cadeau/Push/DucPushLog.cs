#if NET10_0_OR_GREATER
#nullable enable

using System;

namespace Devolutions.Cadeau
{
    /// <summary>
    /// Severity levels reported by <see cref="DucPushLog"/>.
    /// </summary>
    public enum DucPushLogLevel
    {
        Trace,
        Debug,
        Info,
        Warning,
        Error,
    }

    /// <summary>
    /// Optional logging hook for the push code paths (<see cref="DucPushClient"/>).
    /// </summary>
    /// <remarks>
    /// Cadeau intentionally does not take a dependency on any specific logging framework.
    /// Set <see cref="OnLog"/> once at application startup to receive lifecycle and error
    /// messages and forward them to your framework of choice. When <see cref="OnLog"/> is
    /// <see langword="null"/> (the default), log events are dropped.
    /// </remarks>
    /// <example>
    /// <code>
    /// // Bridging to Devolutions.Logging:
    /// var logger = new Devolutions.Logging.Logger("Devolutions.Cadeau.DucPushClient");
    /// DucPushLog.OnLog = (level, message, exception) =>
    /// {
    ///     switch (level)
    ///     {
    ///         case DucPushLogLevel.Error:   logger.Error(message, exception); break;
    ///         case DucPushLogLevel.Warning: logger.Warning(message, exception); break;
    ///         case DucPushLogLevel.Info:    logger.Info(message, exception); break;
    ///         case DucPushLogLevel.Debug:   logger.Debug(message, exception); break;
    ///         case DucPushLogLevel.Trace:   logger.Trace(message, exception); break;
    ///     }
    /// };
    /// </code>
    /// </example>
    public static class DucPushLog
    {
        /// <summary>
        /// Sink invoked for each log event emitted by push code paths. Replace with your own
        /// handler to forward entries to a logging framework. <see langword="null"/> drops events.
        /// </summary>
        public static Action<DucPushLogLevel, string, Exception?>? OnLog { get; set; }

        internal static void Trace(string message) => Emit(DucPushLogLevel.Trace, message, null);
        internal static void Debug(string message) => Emit(DucPushLogLevel.Debug, message, null);
        internal static void Info(string message) => Emit(DucPushLogLevel.Info, message, null);
        internal static void Warning(string message, Exception? exception = null) => Emit(DucPushLogLevel.Warning, message, exception);
        internal static void Error(string message, Exception? exception = null) => Emit(DucPushLogLevel.Error, message, exception);

        private static void Emit(DucPushLogLevel level, string message, Exception? exception)
        {
            try
            {
                OnLog?.Invoke(level, message, exception);
            }
            catch
            {
                // A logging sink must never throw out of the library.
            }
        }
    }
}

#endif
