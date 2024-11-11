#include <iostream>
#include <XmfVpxDecoder.h>

int main()
{


    XmfVpxDecoderConfig config = {};
    XmfVpXDecoder *decoder = XmfVpxDecoder_Create(&config);

    if (!decoder)
    {
        std::cout << "Failed to create decoder!" << std::endl;
        return 1;
    }

    XmfVpxDecoder_Destroy(decoder);
    std::cout << "Test project ran successfully!" << std::endl;
    return 0;
}
