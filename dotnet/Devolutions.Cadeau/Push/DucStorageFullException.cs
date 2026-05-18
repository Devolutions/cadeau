#if NET10_0_OR_GREATER
#nullable enable

using System;

namespace Devolutions.Cadeau
{
    /// <summary>
    /// Thrown when the gateway refuses or terminates a recording push stream because the
    /// recording storage volume is full, not writable, or below the configured minimum
    /// free-space threshold.
    /// </summary>
    public sealed class DucStorageFullException : DucPushException
    {
        public DucStorageFullException(string message) : base(message)
        {
        }

        public DucStorageFullException(string message, Exception? innerException) : base(message, innerException)
        {
        }
    }
}

#endif
