#include <iostream>
#include <Halide.h>

using namespace Halide;

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

#define ycocgy(u, v) (cast<int16_t>(inputY(u, v)))
#define ycocgco(u, v) ((cast<int16_t>(inputCo(u / 2, v / 2)) << 1) - 255)
#define ycocgcg(u, v) ((cast<int16_t>(inputCg(u / 2, v / 2)) << 1) - 255)
	
	void generate()
	{
		t(u, v) = ycocgy(u, v) - (ycocgcg(u, v) >> 1);
		g(u, v) = ycocgcg(u, v) + t(u, v);
		b(u, v) = t(u, v) - (ycocgco(u, v) >> 1);
		r(u, v) = b(u, v) + ycocgco(u, v);
		
		Expr rb = clamp(b(u, v), 0, 255);
		Expr rg = clamp(g(u, v), 0, 255);
		Expr rr = clamp(r(u, v), 0, 255);
		
		outputRgb(c, u, v) =
			select(c == 0, cast<uint8_t>(rb),
			select(c == 1, cast<uint8_t>(rg),
			select(c == 2, cast<uint8_t>(rr), 255)));
	}
		
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
	Func _co { "_co" };
	Func _cg { "_cg" };
	Func t { "t" };

	Input<Func> inputRgb { "inputRgb", UInt(8), 3 };

	Output<Func> outputY { "outputY", UInt(8), 2 };
	Output<Func> outputCo { "outputCo", UInt(8), 2 };
	Output<Func> outputCg { "outputCg", UInt(8), 2 };

#define rgbb(u, v) (cast<int16_t>(inputRgb(0, u, v)))
#define rgbg(u, v) (cast<int16_t>(inputRgb(1, u, v)))
#define rgbr(u, v) (cast<int16_t>(inputRgb(2, u, v)))

#define subsample(x) ((x(u * 2, v * 2) + x(u * 2 + 1, v * 2) + x(u * 2, v * 2 + 1) + x(u * 2 + 1, v * 2 + 1) + 1024) >> 3)
	
	void generate()
	{
		_co(u, v) = rgbr(u, v) - rgbb(u, v);
		t(u, v) = rgbb(u, v) + (_co(u, v) >> 1);
		_cg(u, v) = rgbg(u, v) - t(u, v);
		
		outputY(u, v) = cast<uint8_t>(t(u, v) + (_cg(u, v) >> 1));
		outputCo(u, v) = cast<uint8_t>(subsample(_co));
		outputCg(u, v) = cast<uint8_t>(subsample(_cg));
	}
	
	void schedule()
	{
		outputY.vectorize(u, 16, TailStrategy::GuardWithIf).parallel(v);
		outputCo.vectorize(u, 16, TailStrategy::GuardWithIf).parallel(v);
		outputCg.vectorize(u, 16, TailStrategy::GuardWithIf).parallel(v);
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

HALIDE_REGISTER_GENERATOR(YCoCgR420ToRgb, YCoCgR420ToRgb);
HALIDE_REGISTER_GENERATOR(RgbToYCoCgR420, RgbToYCoCgR420);
HALIDE_REGISTER_GENERATOR(Compare8Stage1, Compare8Stage1);
HALIDE_REGISTER_GENERATOR(Compare32Stage1, Compare32Stage1);
HALIDE_REGISTER_GENERATOR(Downscale2x, Downscale2x);
HALIDE_REGISTER_GENERATOR(Copy, Copy);

int main(int argc, char** argv)
{
	return Halide::Internal::generate_filter_main(argc, argv, std::cerr);
}
