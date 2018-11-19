#include <iostream>
#include <Halide.h>

#define i16(x) (cast<int16_t>(x))
#define u8(x) (cast<uint8_t>(x))

#define rgbb(u, v) (i16(inputRgb(0, u, v)))
#define rgbg(u, v) (i16(inputRgb(1, u, v)))
#define rgbr(u, v) (i16(inputRgb(2, u, v)))
#define rgba(u, v) (i16(inputRgb(3, u, v)))
#define ycocgy(u, v) (i16(inputY(u, v)))
#define ycocgco(u, v) ((i16(inputCo(u / 2, v / 2)) << 1) - 0xFF)
#define ycocgcg(u, v) ((i16(inputCg(u / 2, v / 2)) << 1) - 0xFF)

#define subsample(x) ((x(u * 2, v * 2) + x(u * 2 + 1, v * 2) + x(u * 2, v * 2 + 1) + x(u * 2 + 1, v * 2 + 1) + 1024) >> 3)
#define subsample3(x) ((x(u * 2, v * 2, c) + x(u * 2 + 1, v * 2, c) + x(u * 2, v * 2 + 1, c) + x(u * 2 + 1, v * 2 + 1, c) + 1024) >> 3)

#define input_value(x, t) Input<t> x { #x }

#define input_buffer_u(x, b, d) Input<Func> x { #x, UInt(b), d }
#define input_buffer_i(x, b, d) Input<Func> x { #x, Int(b), d }
#define output_buffer_u(x, b, d) Output<Func> x { #x, UInt(b), d }
#define output_buffer_i(x, b, d) Output<Func> x { #x, Int(b), d }

#define func(x) Func x { #x }
#define var(x) Var x { #x }

#define RegisterGenerator(x) HALIDE_REGISTER_GENERATOR(x, #x)

using namespace Halide;

class YCoCgR420ToRgb : public Generator<YCoCgR420ToRgb>
{
public:
	var(u);
	var(v);
	var(c);
	
	func(rgb);
	func(t);
	func(r);
	func(g);
	func(b);
	
	input_buffer_u(inputY, 8, 2);
	input_buffer_u(inputCo, 8, 2);
	input_buffer_u(inputCg, 8, 2);
	
	output_buffer_u(outputRgb, 8, 3);
	
	void generate()
	{
		t(u, v) = ycocgy(u, v) - (ycocgcg(u, v) >> 1);
		g(u, v) = ycocgcg(u, v) + t(u, v);
		b(u, v) = t(u, v) - (ycocgco(u, v) >> 1);
		r(u, v) = b(u, v) + ycocgco(u, v);
		
		Expr rb = clamp(b(u, v), 0, 255);
		Expr rg = clamp(g(u, v), 0, 255);
		Expr rr = clamp(r(u, v), 0, 255);
		
		outputRgb(c, u, v) = select(c == 0, u8(rb), select(c == 1, u8(rg), select(c == 2, u8(rr), 255)));
	}
		
	void schedule()
	{
		outputRgb.vectorize(u, 16, TailStrategy::GuardWithIf).parallel(v);
	}
};

class RgbToYCoCgR420 : public Generator<RgbToYCoCgR420>
{
public:
	var(u);
	var(v);
	var(c);
	
	func(y);
	func(co);
	func(cg);
	func(_co);
	func(_cg);
	func(t);
	
	input_buffer_u(inputRgb, 8, 3);
	
	output_buffer_u(outputY, 8, 2);
	output_buffer_u(outputCo, 8, 2);
	output_buffer_u(outputCg, 8, 2);
	
	void generate()
	{
		_co(u, v) = rgbr(u, v) - rgbb(u, v);
		t(u, v) = rgbb(u, v) + (_co(u, v) >> 1);
		_cg(u, v) = rgbg(u, v) - t(u, v);
		
		outputY(u, v) = u8(t(u, v) + (_cg(u, v) >> 1));
		outputCo(u, v) = u8(subsample(_co));
		outputCg(u, v) = u8(subsample(_cg));
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
	var(u);
	var(v);
	
	func(vert);
	func(horiz);
	
	input_value(width, int32_t);
	input_value(height, int32_t);
	input_buffer_u(inputRgbOld, 32, 2);
	input_buffer_u(inputRgbNew, 32, 2);
	
	output_buffer_u(outputX, 32, 1);
	output_buffer_u(outputY, 32, 1);
	
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
	var(u);
	var(v);
	
	func(vert);
	func(horiz);
	
	input_value(width, int8_t);
	input_value(height, int8_t);
	input_buffer_u(inputRgbOld, 8, 2);
	input_buffer_u(inputRgbNew, 8, 2);
	
	output_buffer_u(outputX, 8, 1);
	output_buffer_u(outputY, 8, 1);
	
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
	var(u);
	var(v);

	input_buffer_u(input, 32, 2);
	output_buffer_u(output, 32, 2);

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
	var(u);
	var(v);
	var(c);
	
	func(input16);
	
	input_buffer_u(input, 8, 3);
	output_buffer_u(output, 8, 3);
	
	void generate()
	{
		input16(u, v, c) = i16(input(u, v, c));
		
		output(u, v, c) = u8(subsample3(input16));
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
	var(u);
	var(v);
	var(c);
	var(k);
	
	func(kernelX);
	func(kernelY);
	func(kX);
	func(kY);
	func(resizedX);
	func(resizedY);
	
	input_buffer_u(input, 8, 3);
	input_value(scaleFactor, float);
	
	output_buffer_u(output, 8, 3);
	
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

RegisterGenerator(RgbToYCoCgR420);
RegisterGenerator(YCoCgR420ToRgb);
RegisterGenerator(Compare8Stage1);
RegisterGenerator(Compare32Stage1);
RegisterGenerator(Downscale2x);
RegisterGenerator(Copy);

int main(int argc, char **argv)
{
	return Halide::Internal::generate_filter_main(argc, argv, std::cerr);
}
