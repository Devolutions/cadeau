using System;
using System.Linq;
using System.Reflection;
using System.Runtime.CompilerServices;
using System.Threading;
using System.Threading.Tasks;

namespace Devolutions.Cadeau.Vpx.Test
{
    internal static class Program
    {
        private static readonly byte[] RedFrame = Convert.FromBase64String(
            "8BQAnQEqQQHxAABHCIWFiIWEiAICAnWqA/gD+gIGtqT3BoFkn2vbmyc4eyc4eyc4eyc4eyc4eyc4eyc4eyc4eyc4eyc4eyc4eyc4eyc4eyc4eyc4eyc4eyc4eyc4eyc4eyc4eyc4eyc4eyc4eyc4eyc4eyc4eyc4eyc4eyc4eyc4eyc4eyc4eyc4eyc4eyc4eyc4eyc4eyc4eyc4eyc4eyc4eyc4eyc4eyc4eyc4eyKA/v1u8//jmTcwxP+Obf/xYTwOKMj/8VEA");

        private static void Main()
        {
            foreach (Action test in new Action[]
            {
                CopyOutlivesDecoder,
                StaleImagesThrow,
                DisposeWaitsForImageReads,
                CopyRacesWithDisposal,
                OrphanedImageSurvivesGc,
            })
            {
                test();
                Console.WriteLine($"PASS {test.Method.Name}");
            }
        }

        private static void CopyOutlivesDecoder()
        {
            using XmfVpxDecoder decoder = new XmfVpxDecoder(new XmfVpxDecoderConfig { Threads = 1 });
            using XmfVpxImage image = DecodeImage(decoder);
            XmfVpxImageCopy copy = image.Copy();
            AssertCopy(copy);
            Assert(decoder.Decode(RedFrame), "Decode failed");
            decoder.Dispose();
            image.Dispose();
            GC.Collect();
            GC.WaitForPendingFinalizers();
            AssertCopy(copy);
        }

        private static void StaleImagesThrow()
        {
            using XmfVpxDecoder decoder = new XmfVpxDecoder(new XmfVpxDecoderConfig { Threads = 1 });
            using XmfVpxImage stale = DecodeImage(decoder);
            using XmfVpxImage current = DecodeImage(decoder);
            AssertThrows<InvalidOperationException>(() => stale.Copy());
            decoder.Handle.Dispose();
            AssertThrows<InvalidOperationException>(() => current.Copy());
            AssertThrows<InvalidOperationException>(() => current.GetPlane(XmfVpxPlane.Y));
            AssertThrows<InvalidOperationException>(() => current.GetStride(XmfVpxPlane.Y));
        }

        private static void DisposeWaitsForImageReads()
        {
            using XmfVpxDecoder decoder = new XmfVpxDecoder(new XmfVpxDecoderConfig { Threads = 1 });
            using XmfVpxImage image = DecodeImage(decoder);
            object syncRoot = typeof(XmfVpxDecoder).GetField("SyncRoot", BindingFlags.Instance | BindingFlags.NonPublic).GetValue(decoder);
            using ManualResetEventSlim started = new ManualResetEventSlim();
            Task disposal;
            lock (syncRoot)
            {
                disposal = Task.Run(() =>
                {
                    started.Set();
                    image.Dispose();
                });
                Assert(started.Wait(TimeSpan.FromSeconds(5)), "Disposal did not start");
                Assert(!disposal.Wait(TimeSpan.FromMilliseconds(250)), "Dispose bypassed the image read lock");
                AssertCopy(image.Copy());
                decoder.Handle.Dispose();
                Assert(!decoder.Handle.IsClosed, "Decoder memory was released during an image read");
            }
            Assert(disposal.Wait(TimeSpan.FromSeconds(5)), "Disposal did not finish");
            Assert(decoder.Handle.IsClosed, "Decoder memory was not released with the last image");
            AssertThrows<ObjectDisposedException>(() => image.Copy());
        }

        private static void CopyRacesWithDisposal()
        {
            for (int iteration = 0; iteration < 256; iteration++)
            {
                using XmfVpxDecoder decoder = new XmfVpxDecoder(new XmfVpxDecoderConfig { Threads = 1 });
                using XmfVpxImage image = DecodeImage(decoder);
                using ManualResetEventSlim start = new ManualResetEventSlim();
                Task copy = Task.Run(() =>
                {
                    start.Wait();
                    try
                    {
                        AssertCopy(image.Copy());
                    }
                    catch (ObjectDisposedException) { }
                    catch (InvalidOperationException error) when (error.Message.StartsWith("The decoded image is no longer valid:", StringComparison.Ordinal)) { }
                });
                Task disposal = Task.Run(() =>
                {
                    start.Wait();
                    decoder.Handle.Dispose();
                    image.Dispose();
                });
                start.Set();
                Assert(Task.WaitAll(new[] { copy, disposal }, TimeSpan.FromSeconds(5)), "Copy/disposal race timed out");
            }
        }

        private static void OrphanedImageSurvivesGc()
        {
            using XmfVpxImage image = CreateOrphanedImage();
            GC.Collect();
            GC.WaitForPendingFinalizers();
            AssertCopy(image.Copy());
        }

        [MethodImpl(MethodImplOptions.NoInlining)]
        private static XmfVpxImage CreateOrphanedImage()
        {
            XmfVpxDecoder decoder = new XmfVpxDecoder(new XmfVpxDecoderConfig { Threads = 1 });
            return DecodeImage(decoder);
        }

        private static XmfVpxImage DecodeImage(XmfVpxDecoder decoder)
        {
            Assert(decoder.Decode(RedFrame), "Decode failed");
            return decoder.GetNextFrame() ?? throw new InvalidOperationException("No decoded image");
        }

        private static void AssertCopy(XmfVpxImageCopy copy)
        {
            Assert(copy.Width == 321 && copy.Height == 241, "Wrong image dimensions");
            Assert(copy.Format == XmfVpxImageFormat.I420, "Wrong image format");
            Assert(copy.ColorSpace == XmfVpxColorSpace.Unknown && copy.ColorRange == XmfVpxColorRange.Studio, "Wrong color metadata");
            foreach ((XmfVpxImagePlane plane, int width, int height, byte value) in new[]
            {
                (copy.Y, 321, 241, (byte)81),
                (copy.U, 161, 121, (byte)90),
                (copy.V, 161, 121, (byte)240),
            })
            {
                Assert(plane.Width == width && plane.Height == height && plane.Stride == width, "Wrong plane dimensions");
                Assert(plane.Data.Length == width * height && plane.Data.All(pixel => pixel == value), "Wrong plane pixels");
            }
        }

        private static void AssertThrows<T>(Action action) where T : Exception
        {
            try
            {
                action();
            }
            catch (T)
            {
                return;
            }
            throw new InvalidOperationException($"Expected {typeof(T).Name}");
        }

        private static void Assert(bool condition, string message)
        {
            if (!condition)
            {
                throw new InvalidOperationException(message);
            }
        }
    }
}