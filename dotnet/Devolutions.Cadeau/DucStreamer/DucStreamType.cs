namespace Devolutions.Cadeau
{
    public enum DucStreamType : ushort
    {
        Unknown = 0,

        BitmapFrame = 147,

        /// <summary>
        /// BitmapUpdate is deprecated and should not be used.
        /// </summary>
        BitmapUpdate = 148,

        Ssh = 149,

        /// <summary>
        /// Wayk is deprecated and should not be used.
        /// </summary>
        Wayk = 150,

        /// <summary>
        /// Raw file bytes
        /// </summary>
        Raw = 175
    }
}
