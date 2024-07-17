
#include <XmfMuxer.h>


int main() {
	auto muxer = XmfWebMMuxer_New();
	const char* input = "C:\\Users\\jou\\code\\cadeau\\media\\non-seekable.webm";
	const char* output = "C:\\Users\\jou\\code\\cadeau\\media\\non-seekable-muxed.webm";

	auto result = XmfWebMMuxer_Remux(muxer, input, output);
	return result;
}