using System;
using System.Collections.Generic;
using System.Text;

namespace Devolutions.Cadeau
{
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
}
