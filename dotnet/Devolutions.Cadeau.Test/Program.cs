﻿using System;
using System.IO;
using System.Threading;
using System.Collections.Generic;
using System.Runtime.InteropServices;

using Devolutions.Cadeau;

namespace Devolutions.Cadeau.Test
{
    class Program
    {
        static void TestRecorder()
        {
            uint frameWidth = 1280;
            uint frameHeight = 720;
            uint frameRate = 10;

            XmfRecorder recorder = new XmfRecorder();
            string currentDir = Directory.GetCurrentDirectory();
            string rootDir = Directory.GetParent(currentDir).Parent.ToString();
            string mediaDir = Path.Join(rootDir, "media");
            string mediaFile = Path.Join(mediaDir, "window_dragging.rgb");
            Console.WriteLine("media dir: {0}", mediaDir);
            string outputFile = mediaFile.Replace(".rgb", ".webm");
            recorder.SetFileName(outputFile);
            recorder.SetFrameSize(frameWidth, frameHeight);
            recorder.SetFrameRate(frameRate);
            recorder.Init();

            uint surfaceWidth = frameWidth;
            uint surfaceHeight = frameHeight;
            uint surfaceStep = surfaceWidth * 4;
            uint surfaceSize = surfaceHeight * surfaceStep;

            byte[] fileData = File.ReadAllBytes(mediaFile);
            uint fileSize = (uint) fileData.Length;

            long offset = 0;

            unsafe {
                fixed (byte* ptr = fileData)
                {
                    while (offset < fileSize) {
                        uint timeout = recorder.GetTimeout();
                        Thread.Sleep((int) timeout);
                        IntPtr buffer = new IntPtr(&ptr[offset]);
                        recorder.UpdateFrame(buffer, 0, 0, surfaceWidth, surfaceHeight, surfaceStep);
                        recorder.Timeout();
                        offset += surfaceSize;
                    }
                }
            }
        }

        static void TestBipBuffer()
        {
            XmfBipBuffer bb = new XmfBipBuffer();
            nuint bufferSize = bb.GetBufferSize();

            Console.WriteLine("BufferSize: {0}", bufferSize);

            uint frameWidth = 1280;
            uint frameHeight = 720;
            uint frameRate = 10;

            XmfRecorder recorder = new XmfRecorder();
            string currentDir = Directory.GetCurrentDirectory();
            string rootDir = Directory.GetParent(currentDir).Parent.ToString();
            string mediaDir = Path.Join(rootDir, "media");
            string mediaFile = Path.Join(mediaDir, "window_dragging.rgb");
            Console.WriteLine("media dir: {0}", mediaDir);
            string outputFile = mediaFile.Replace(".rgb", ".webm");
            recorder.SetFileName(outputFile);
            recorder.SetFrameSize(frameWidth, frameHeight);
            recorder.SetFrameRate(frameRate);
            recorder.Init();

            uint surfaceWidth = frameWidth;
            uint surfaceHeight = frameHeight;
            uint surfaceStep = surfaceWidth * 4;
            uint surfaceSize = surfaceHeight * surfaceStep;

            byte[] fileData = File.ReadAllBytes(mediaFile);
            uint fileSize = (uint) fileData.Length;

            long offset = 0;

            unsafe {
                fixed (byte* ptr = fileData)
                {
                    while (offset < fileSize) {
                        IntPtr surfaceData = new IntPtr(&ptr[offset]);
                        bb.Grow(bb.GetUsedSize() + surfaceSize);
                        int status = bb.Write(surfaceData, surfaceSize);
                        offset += surfaceSize;
                    }
                }

                byte[] surfaceBytes = new byte[surfaceSize];

                fixed (byte* ptr = surfaceBytes)
                {
                    IntPtr surfaceData = new IntPtr(ptr);

                    while (bb.GetUsedSize() > 0) {
                        uint timeout = recorder.GetTimeout();
                        Thread.Sleep((int) timeout);
                        int status = bb.Read(surfaceData, surfaceSize);
                        recorder.UpdateFrame(surfaceData, 0, 0, surfaceWidth, surfaceHeight, surfaceStep);
                        recorder.Timeout();
                    }
                }
            }
        }

        static ulong GetTickCount()
        {
            return (ulong) (DateTime.UtcNow.Ticks / TimeSpan.TicksPerMillisecond);
        }

        static void TestMkvStream()
        {
            uint frameWidth = 1280;
            uint frameHeight = 720;
            uint frameRate = 10;

            bool manualTime = true;
            ulong currentTime = GetTickCount();
            XmfMkvStream mkvStream = new XmfMkvStream();

            XmfRecorder recorder = new XmfRecorder();
            string currentDir = Directory.GetCurrentDirectory();
            string rootDir = Directory.GetParent(currentDir).Parent.ToString();
            string mediaDir = Path.Join(rootDir, "media");
            string mediaFile = Path.Join(mediaDir, "window_dragging.rgb");
            Console.WriteLine("media dir: {0}", mediaDir);
            string outputFile = mediaFile.Replace(".rgb", ".webm");
            recorder.SetBipBuffer(mkvStream.bb.Handle);
            recorder.SetFrameSize(frameWidth, frameHeight);
            recorder.SetFrameRate(frameRate);

            if (manualTime) {
                recorder.SetCurrentTime(currentTime);
            }

            recorder.Init();

            uint surfaceWidth = frameWidth;
            uint surfaceHeight = frameHeight;
            uint surfaceStep = surfaceWidth * 4;
            uint surfaceSize = surfaceHeight * surfaceStep;

            byte[] fileData = File.ReadAllBytes(mediaFile);
            uint fileSize = (uint) fileData.Length;

            long offset = 0;

            unsafe {
                fixed (byte* ptr = fileData)
                {
                    while (offset < fileSize) {
                        uint timeout = recorder.GetTimeout();
                        Console.WriteLine("currentTime: {0}, timeout: {1}", currentTime, timeout);

                        if (manualTime) {
                            currentTime += timeout;
                            recorder.SetCurrentTime(currentTime);
                        } else {
                            Thread.Sleep((int) timeout);
                        }
                        
                        IntPtr buffer = new IntPtr(&ptr[offset]);
                        recorder.UpdateFrame(buffer, 0, 0, surfaceWidth, surfaceHeight, surfaceStep);
                        recorder.Timeout();
                        offset += surfaceSize;
                    }
                }
            }

            MemoryStream memStream = new MemoryStream();
            mkvStream.CopyTo(memStream);
            byte[] mkvBytes = memStream.ToArray();

            Console.WriteLine("MKV bytes: {0}", mkvBytes.Length);

            FileStream fs = new FileStream(outputFile, FileMode.Create, FileAccess.Write);
            fs.Write(mkvBytes);
            fs.Flush();
        }

        static void TestImageFile()
        {
            string currentDir = Directory.GetCurrentDirectory();
            string rootDir = Directory.GetParent(currentDir).Parent.ToString();
            string mediaDir = Path.Join(rootDir, "media");
            string inputFile = Path.Join(mediaDir, "winlogon_unlock.png");
            string outputFile = Path.Join(mediaDir, "output.bmp");

            unsafe {
                IntPtr data = IntPtr.Zero;
                uint width = 0;
                uint height = 0;
                uint step = 0;

                if (XmfImage.LoadFile(inputFile, ref data, ref width, ref height, ref step))
                {
                    Console.WriteLine("image: {0}x{1}, step: {2}", width, height, step);
                    if (!XmfImage.SaveFile(outputFile, data, width, height, step)) {
                        Console.WriteLine("failed to save file {0}", outputFile);
                    }
                    XmfImage.FreeData(data);
                }
            }
        }

        static (string[], List<string[]>) ParsePsvFile(string filePath)
        {
            string[] headers = null;
            List<string[]> records = new List<string[]>();
            using (StreamReader reader = new StreamReader(filePath))
            {
                while (!reader.EndOfStream)
                {
                    string line = reader.ReadLine();
                    string[] fields = line.Split('|');

                    if (headers == null) {
                        headers = fields;
                    } else {
                        records.Add(fields);
                    }
                }
            }
            return (headers, records);
        }

        static void TestTranscode()
        {
            string currentDir = Directory.GetCurrentDirectory();
            string rootDir = Directory.GetParent(currentDir).Parent.ToString();
            string mediaDir = Path.Join(rootDir, "media");
            string capturePath = Path.Join(mediaDir, "capture_sample");
            string captureFile = Path.Join(capturePath, "frame_meta.psv");
            string videoFile = Path.Join(capturePath, "video.webm");

            (string[] headers, List<string[]> records) = ParsePsvFile(captureFile);

            Console.WriteLine(string.Join("|", headers));

            uint frameWidth = 1920;
            uint frameHeight = 1080;
            uint frameRate = 10;

            ulong baseTime = GetTickCount();
            XmfRecorder recorder = new XmfRecorder();
            recorder.SetFileName(videoFile);
            recorder.SetFrameSize(frameWidth, frameHeight);
            recorder.SetFrameRate(frameRate);
            recorder.SetCurrentTime(baseTime);
            recorder.Init();

            foreach (string[] record in records)
            {
                ulong frameTime = ulong.Parse(record[0]);
                ulong currentTime = baseTime + frameTime;
                string frameSize = record[1];
                string frameFile = record[2];
                string inputFile = Path.Join(capturePath, frameFile);

                unsafe {
                    IntPtr data = IntPtr.Zero;
                    uint width = 0;
                    uint height = 0;
                    uint step = 0;

                    if (XmfImage.LoadFile(inputFile, ref data, ref width, ref height, ref step))
                    {
                        Console.WriteLine("image: {0}x{1}, time: {2}", width, height, frameTime);

                        recorder.SetCurrentTime(currentTime);
                        recorder.UpdateFrame(data, 0, 0, width, height, step);
                        recorder.Timeout();

                        XmfImage.FreeData(data);
                    }
                }
            }

            recorder.Uninit();
        }

        static void Main(string[] args)
        {
            //TestRecorder();
            //TestBipBuffer();
            //TestMkvStream();
            //TestImageFile();
            TestTranscode();
        }
    }
}
