#if NET10_0_OR_GREATER
#nullable enable

using System;

namespace Devolutions.Cadeau
{
    /// <summary>
    /// Base type for errors surfaced by <see cref="DucPushClient"/> when the gateway rejects or
    /// terminates a recording push stream for a known, reportable reason.
    /// </summary>
    /// <remarks>
    /// Catch <see cref="DucPushException"/> to handle any such push-protocol error generically;
    /// catch a derived type (e.g. <see cref="DucStorageFullException"/>) to react to a specific
    /// condition. New reportable conditions should be added as further derived types so callers can
    /// keep distinguishing them without breaking existing handling.
    /// </remarks>
    public class DucPushException : Exception
    {
        public DucPushException(string message) : base(message)
        {
        }

        public DucPushException(string message, Exception? innerException) : base(message, innerException)
        {
        }
    }
}

#endif
