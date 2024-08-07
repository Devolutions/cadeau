#include <iostream>
#include <Halide.h>

using namespace Halide;

class RgbToYCoCgR : public Generator<RgbToYCoCgR>
{
public:
    Var u { "u" };
    Var v { "v" };
    Var c { "c" };

    Func y { "y" };
    Func co { "co" };
    Func cg { "cg" };
    Func t { "t" };

    Input<Func> inputRgb { "inputRgb", UInt(8), 3 };

    Output<Func> outputY { "outputY", Int(16), 2 };
    Output<Func> outputCo { "outputCo", Int(16), 2 };
    Output<Func> outputCg { "outputCg", Int(16), 2 };

#define B(u, v) (cast<int16_t>(inputRgb(0, u, v)))
#define G(u, v) (cast<int16_t>(inputRgb(1, u, v)))
#define R(u, v) (cast<int16_t>(inputRgb(2, u, v)))

    void generate()
    {
        co(u, v) = R(u, v) - B(u, v); // Co = R - B;
        t(u, v) = B(u, v) + (co(u, v) >> 1); // t = B + (Co >> 1);
        cg(u, v) = G(u, v) - t(u, v); // Cg = G - t;
        y(u, v) = t(u, v) + (cg(u, v) >> 1); // Y = t + (Cg >> 1);

        outputY(u, v) = cast<int16_t>(y(u, v) - 128); // [-128, 127]
        outputCo(u, v) = cast<int16_t>(co(u, v)); // [-256, 255]
        outputCg(u, v) = cast<int16_t>(cg(u, v)); // [-256, 255]
    }

#undef B
#undef G
#undef R

    void schedule()
    {
        outputY.vectorize(u, 16, TailStrategy::GuardWithIf).parallel(v);
        outputCo.vectorize(u, 16, TailStrategy::GuardWithIf).parallel(v);
        outputCg.vectorize(u, 16, TailStrategy::GuardWithIf).parallel(v);
    }
};

class YCoCgRToRgb : public Generator<YCoCgRToRgb>
{
public:
    Var u { "u" };
    Var v { "v" };
    Var c { "c" };

    Func rgb { "rgb" };
    Func t { "t" };
    Func r { "r" };
    Func g { "g" };
    Func b { "b" };

    Input<Func> inputY { "inputY", Int(16), 2};
    Input<Func> inputCo { "inputCo", Int(16), 2};
    Input<Func> inputCg { "inputCg", Int(16), 2};

    Output<Func> outputRgb { "outputRgb", UInt(8), 3 };

#define Y(u, v) (cast<int16_t>(inputY(u, v) + 128))
#define Co(u, v) (cast<int16_t>(inputCo(u, v)))
#define Cg(u, v) (cast<int16_t>(inputCg(u, v)))

    void generate()
    {
        t(u, v) = Y(u, v) - (Cg(u, v) >> 1);
        g(u, v) = Cg(u, v) + t(u, v);
        b(u, v) = t(u, v) - (Co(u, v) >> 1);
        r(u, v) = b(u, v) + Co(u, v);

        Expr rb = clamp(b(u, v), 0, 255);
        Expr rg = clamp(g(u, v), 0, 255);
        Expr rr = clamp(r(u, v), 0, 255);

        outputRgb(c, u, v) =
            select(c == 0, cast<uint8_t>(rb),
            select(c == 1, cast<uint8_t>(rg),
            select(c == 2, cast<uint8_t>(rr), 255)));
    }

#undef Y
#undef Co
#undef Cg

    void schedule()
    {
        outputRgb.vectorize(u, 16, TailStrategy::GuardWithIf).parallel(v);
    }
};

class RgbToYCoCgR420 : public Generator<RgbToYCoCgR420>
{
public:
    Var u { "u" };
    Var v { "v" };
    Var c { "c" };

    Func y { "y" };
    Func co { "co" };
    Func cg { "cg" };
    Func t { "t" };

    Input<Func> inputRgb { "inputRgb", UInt(8), 3 };

    Output<Func> outputY { "outputY", UInt(8), 2 };
    Output<Func> outputCo { "outputCo", UInt(8), 2 };
    Output<Func> outputCg { "outputCg", UInt(8), 2 };

#define B(u, v) (cast<int16_t>(inputRgb(0, u, v)))
#define G(u, v) (cast<int16_t>(inputRgb(1, u, v)))
#define R(u, v) (cast<int16_t>(inputRgb(2, u, v)))

#define subsample(x) ((x(u * 2, v * 2) + x(u * 2 + 1, v * 2) + x(u * 2, v * 2 + 1) + x(u * 2 + 1, v * 2 + 1) + 1024) >> 3)

    void generate()
    {
        co(u, v) = R(u, v) - B(u, v);
        t(u, v) = B(u, v) + (co(u, v) >> 1);
        cg(u, v) = G(u, v) - t(u, v);
        y(u, v) = t(u, v) + (cg(u, v) >> 1);

        outputY(u, v) = cast<uint8_t>(y(u, v));
        outputCo(u, v) = cast<uint8_t>(subsample(co));
        outputCg(u, v) = cast<uint8_t>(subsample(cg));
    }

#undef B
#undef G
#undef R

    void schedule()
    {
        outputY.vectorize(u, 16, TailStrategy::GuardWithIf).parallel(v);
        outputCo.vectorize(u, 16, TailStrategy::GuardWithIf).parallel(v);
        outputCg.vectorize(u, 16, TailStrategy::GuardWithIf).parallel(v);
    }
};

class YCoCgR420ToRgb : public Generator<YCoCgR420ToRgb>
{
public:
    Var u { "u" };
    Var v { "v" };
    Var c { "c" };

    Func rgb { "rgb" };
    Func t { "t" };
    Func r { "r" };
    Func g { "g" };
    Func b { "b" };

    Input<Func> inputY { "inputY", UInt(8), 2};
    Input<Func> inputCo { "inputCo", UInt(8), 2};
    Input<Func> inputCg { "inputCg", UInt(8), 2};

    Output<Func> outputRgb { "outputRgb", UInt(8), 3 };

#define Y(u, v) (cast<int16_t>(inputY(u, v)))
#define Co(u, v) ((cast<int16_t>(inputCo(u / 2, v / 2)) << 1) - 255)
#define Cg(u, v) ((cast<int16_t>(inputCg(u / 2, v / 2)) << 1) - 255)
    
    void generate()
    {
        t(u, v) = Y(u, v) - (Cg(u, v) >> 1);
        g(u, v) = Cg(u, v) + t(u, v);
        b(u, v) = t(u, v) - (Co(u, v) >> 1);
        r(u, v) = b(u, v) + Co(u, v);
        
        Expr rb = clamp(b(u, v), 0, 255);
        Expr rg = clamp(g(u, v), 0, 255);
        Expr rr = clamp(r(u, v), 0, 255);
        
        outputRgb(c, u, v) =
            select(c == 0, cast<uint8_t>(rb),
            select(c == 1, cast<uint8_t>(rg),
            select(c == 2, cast<uint8_t>(rr), 255)));
    }

#undef Y
#undef Co
#undef Cg
        
    void schedule()
    {
        outputRgb.vectorize(u, 16, TailStrategy::GuardWithIf).parallel(v);
    }
};

class Compare32Stage1 : public Generator<Compare32Stage1>
{
public:
    Var u { "u" };
    Var v { "v" };

    Func vert { "vert" };
    Func horiz { "horiz" };

    Input<int32_t> width { "width" };
    Input<int32_t> height { "height" };

    Input<Func> inputRgbOld { "inputRgbOld", UInt(32), 2 };
    Input<Func> inputRgbNew { "inputRgbNew", UInt(32), 2 };

    Output<Func> outputX { "outputX", UInt(32), 1 };
    Output<Func> outputY { "outputY", UInt(32), 1 };
    
    void generate()
    {
        RDom rX(0, width);
        RDom rY(0, height);
        
        outputX(u) = maximum(inputRgbNew(u, rY) ^ inputRgbOld(u, rY));
        outputY(v) = maximum(inputRgbNew(rX, v) ^ inputRgbOld(rX, v));
    }
    
    void schedule()
    {
        outputX.vectorize(u, 16, TailStrategy::GuardWithIf).parallel(u);
        outputY.vectorize(v, 16, TailStrategy::GuardWithIf).parallel(v);
    }
};

class Compare8Stage1 : public Generator<Compare8Stage1>
{
public:
    Var u { "u" };
    Var v { "v" };

    Func vert { "vert" };
    Func horiz { "horiz" };

    Input<int8_t> width { "width" };
    Input<int8_t> height { "height" };

    Input<Func> inputRgbOld { "inputRgbOld", UInt(8), 2 };
    Input<Func> inputRgbNew { "inputRgbNew", UInt(8), 2 };

    Output<Func> outputX { "outputX", UInt(8), 1 };
    Output<Func> outputY { "outputY", UInt(8), 1 };
    
    void generate()
    {
        RDom rX(0, width);
        RDom rY(0, height);
        
        outputX(u) = maximum(inputRgbNew(u, rY) ^ inputRgbOld(u, rY));
        outputY(v) = maximum(inputRgbNew(rX, v) ^ inputRgbOld(rX, v));
    }
    
    void schedule()
    {
        outputX.vectorize(u, 16, TailStrategy::GuardWithIf).parallel(u);
        outputY.vectorize(v, 16, TailStrategy::GuardWithIf).parallel(v);
    }
};

class Copy : public Generator<Copy>
{
public:
    Var u { "u" };
    Var v { "v" };

    Input<Func> input { "input", UInt(32), 2 };
    Output<Func> output { "output", UInt(32), 2 };

    void generate()
    {
        output(u, v) = input(u, v);
    }

    void schedule()
    {
        output.vectorize(u, 16, TailStrategy::GuardWithIf).parallel(v);
    }
};

class Downscale2x : public Generator<Downscale2x>
{
public:
    Var u { "u" };
    Var v { "v" };
    Var c { "c" };

    Func input16 { "input16" };

    Input<Func> input { "input", UInt(8), 3 };
    Output<Func> output { "output", UInt(8), 3 };

#define subsample3(x) ((x(u * 2, v * 2, c) + x(u * 2 + 1, v * 2, c) + x(u * 2, v * 2 + 1, c) + x(u * 2 + 1, v * 2 + 1, c) + 1024) >> 3)
    
    void generate()
    {
        input16(u, v, c) = cast<int16_t>(input(u, v, c));
        
        output(u, v, c) = cast<uint8_t>(subsample3(input16));
    }
    
    void schedule()
    {
        output.vectorize(u, 16, TailStrategy::GuardWithIf).parallel(v);
    }
};

Expr kernel_linear(Expr x)
{
    Expr xx = abs(x);
    return select(xx < 1.0f, 1.0f - xx, 0.0f);
}

class Scale : public Generator<Scale>
{
public:
    Var u { "u" };
    Var v { "v" };
    Var c { "c" };
    Var k { "k" };

    Func kernelX { "kernelX" };
    Func kernelY { "kernelY" };
    Func kX { "kX" };
    Func kY { "kY" };
    Func resizedX { "resizedX" };
    Func resizedY { "resizedY" };

    Input<Func> input { "input", UInt(8), 3 };
    Input<float> scaleFactor { "scaleFactor" };

    Output<Func> output { "output", UInt(8), 3 };
    
    void generate()
    {    
        Expr kernelScaling = min(scaleFactor, 1.0f);
        Expr sourceX = (u + 0.5f) / scaleFactor;
        Expr sourceY = (v + 0.5f) / scaleFactor;
        
        Expr beginX = cast<int>(sourceX - 0.5f);
        Expr beginY = cast<int>(sourceY - 0.5f);
        
        RDom domX(0, 3, "domX");
        RDom domY(0, 3, "domY");
        {
            kX(u, k) = kernel_linear((k + beginX + sourceX) * kernelScaling);
            kY(u, k) = kernel_linear((k + beginY + sourceY) * kernelScaling);
            kernelX(u, k) = kX(u, k) / sum(kX(u, domX));
            kernelY(u, k) = kY(v, k) / sum(kY(v, domY));
        }
        
        resizedX(u, v, c) = sum(kernelX(u, domX) * cast<float>(input(domX + beginX, v, c)));
        resizedY(u, v, c) = sum(kernelY(v, domY) * resizedX(u, domY + beginY, c));
        
        output(u, v, c) = clamp(resizedY(u, v, c), 0.0f, 1.0f);
    }
    
    void schedule()
    {
        output.vectorize(u, 16, TailStrategy::GuardWithIf).parallel(v);
    }
};

HALIDE_REGISTER_GENERATOR(RgbToYCoCgR, RgbToYCoCgR);
HALIDE_REGISTER_GENERATOR(YCoCgRToRgb, YCoCgRToRgb);
HALIDE_REGISTER_GENERATOR(RgbToYCoCgR420, RgbToYCoCgR420);
HALIDE_REGISTER_GENERATOR(YCoCgR420ToRgb, YCoCgR420ToRgb);
HALIDE_REGISTER_GENERATOR(Compare8Stage1, Compare8Stage1);
HALIDE_REGISTER_GENERATOR(Compare32Stage1, Compare32Stage1);
HALIDE_REGISTER_GENERATOR(Downscale2x, Downscale2x);
HALIDE_REGISTER_GENERATOR(Copy, Copy);

int main(int argc, char** argv)
{
    return Halide::Internal::generate_filter_main(argc, argv, std::cerr);
}
