// Screenshot ownership/size/request/encoding contracts. No disk images.
#include "gpu/screenshot_readback.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdio>
#include <cstdlib>
#if defined(_WIN32)
#include <wincodec.h>
#include <wrl/client.h>
#endif
using namespace bd::gpu;
void CheckScreenshotContracts() {
  assert(!ScreenshotPlan::Make(0,1) && !ScreenshotPlan::Make(1,0));
  assert(!ScreenshotPlan::Make(~0u,1) && !ScreenshotPlan::Make(8192,8192));
  const auto plan = ScreenshotPlan::Make(65,3);
  assert(plan && plan->pitch == 512 && plan->bytes == 1536);
  auto request = ParseScreenshotProbe("1 300 420\n");
  assert(request && request->Accept(300) && request->Accept(420) && !request->Accept(299) && !request->Accept(421));
  for (const auto bad : {"", "1 300", "0 300 420", "1 0 120", "1 300 421", "1 420 300",
                         "-1 300 420", "1 -300 420", "1 300 420 garbage", "1 300 420 1", "1 4294967296 4294967296"})
    assert(!ParseScreenshotProbe(bad));
  assert(!ParseScreenshotProbe(std::string(65,'1')));
  Capture capture; capture.width = 32; capture.height = 16;
  capture.rgba.resize(capture.width*capture.height*4);
  for (size_t i=0;i<capture.rgba.size();i+=4) { capture.rgba[i] = 255; capture.rgba[i+3] = 255; }
  assert(EncodeJpeg(capture,64).empty() && EncodeJpeg(capture,0).empty() && EncodeJpeg(capture,2u<<20).empty());
  const auto encoded = EncodeJpeg(capture,4096);
#if defined(_WIN32)
  assert(encoded.size() > 4 && encoded.size() <= 4096 && encoded[0] == 255 && encoded[1] == 216);
  assert(encoded[encoded.size()-2] == 255 && encoded.back() == 217);
  using Microsoft::WRL::ComPtr;
  const auto apartment = CoInitializeEx(nullptr,COINIT_MULTITHREADED);
  assert(SUCCEEDED(apartment));
  {
    ComPtr<IWICImagingFactory> factory;
    assert(SUCCEEDED(CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&factory))));
    ComPtr<IWICStream> stream;
    assert(SUCCEEDED(factory->CreateStream(&stream)));
    assert(SUCCEEDED(stream->InitializeFromMemory(const_cast<BYTE *>(encoded.data()),DWORD(encoded.size()))));
    ComPtr<IWICBitmapDecoder> decoder;
    assert(SUCCEEDED(factory->CreateDecoderFromStream(stream.Get(),nullptr,WICDecodeMetadataCacheOnLoad,&decoder)));
    ComPtr<IWICBitmapFrameDecode> frame;
    assert(SUCCEEDED(decoder->GetFrame(0,&frame)));
    UINT width=0,height=0; assert(SUCCEEDED(frame->GetSize(&width,&height)) && width == capture.width && height == capture.height);
    ComPtr<IWICFormatConverter> converted;
    assert(SUCCEEDED(factory->CreateFormatConverter(&converted)));
    assert(SUCCEEDED(converted->Initialize(frame.Get(),GUID_WICPixelFormat32bppRGBA,WICBitmapDitherTypeNone,nullptr,0,WICBitmapPaletteTypeCustom)));
    std::vector<uint8_t> rgba(capture.rgba.size());
    assert(SUCCEEDED(converted->CopyPixels(nullptr,width*4,UINT(rgba.size()),rgba.data())));
    for (size_t i=0;i<rgba.size();i+=4) assert(rgba[i] >= 250 && rgba[i+1] <= 5 && rgba[i+2] <= 5 && rgba[i+3] == 255);
  }
  Capture detailed; detailed.width = 1920; detailed.height = 1080;
  detailed.rgba.resize(size_t(detailed.width)*detailed.height*4);
  for (uint32_t y=0;y<detailed.height;++y) for (uint32_t x=0;x<detailed.width;++x) {
    auto *pixel = detailed.rgba.data()+(size_t(y)*detailed.width+x)*4;
    const uint32_t pattern = ((x/8)*1103515245u)^((y/8)*2654435761u);
    pixel[0] = uint8_t(pattern); pixel[1] = uint8_t(pattern>>8); pixel[2] = uint8_t(pattern>>16); pixel[3] = 255;
  }
  const auto large = EncodeJpeg(detailed,256u*1024);
  std::printf("Full-resolution JPEG bytes=%zu\n",large.size());
  std::fflush(stdout);
  if (!(large.size() > 65536 && large.size() <= 256u*1024 &&
        large[large.size()-2] == 255 && large.back() == 217)) {
    std::fputs("Full-resolution JPEG extent or EOI failed\n",stderr); std::exit(1);
  }
  {
    ComPtr<IWICImagingFactory> factory;
    assert(SUCCEEDED(CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&factory))));
    ComPtr<IWICStream> stream;
    assert(SUCCEEDED(factory->CreateStream(&stream)));
    assert(SUCCEEDED(stream->InitializeFromMemory(const_cast<BYTE *>(large.data()),DWORD(large.size()))));
    ComPtr<IWICBitmapDecoder> decoder;
    assert(SUCCEEDED(factory->CreateDecoderFromStream(stream.Get(),nullptr,WICDecodeMetadataCacheOnLoad,&decoder)));
    ComPtr<IWICBitmapFrameDecode> frame;
    assert(SUCCEEDED(decoder->GetFrame(0,&frame)));
    UINT width=0,height=0; assert(SUCCEEDED(frame->GetSize(&width,&height)) && width == detailed.width && height == detailed.height);
    ComPtr<IWICFormatConverter> converted;
    assert(SUCCEEDED(factory->CreateFormatConverter(&converted)));
    assert(SUCCEEDED(converted->Initialize(frame.Get(),GUID_WICPixelFormat32bppRGBA,WICBitmapDitherTypeNone,nullptr,0,WICBitmapPaletteTypeCustom)));
    std::vector<uint8_t> rgba(detailed.rgba.size());
    assert(SUCCEEDED(converted->CopyPixels(nullptr,width*4,UINT(rgba.size()),rgba.data())));
    int maximum_error = 0;
    for (uint32_t y=4;y<height;y+=8) for (uint32_t x=4;x<width;x+=8) {
      const size_t offset = (size_t(y)*width+x)*4;
      for (size_t channel=0;channel<3;++channel) {
        const int error = std::abs(int(rgba[offset+channel])-int(detailed.rgba[offset+channel]));
        if (error > maximum_error) maximum_error = error;
      }
      assert(rgba[offset+3] == 255);
    }
    std::printf("Full-resolution JPEG maximum block-centre error=%d (limit 12)\n",maximum_error);
    if (maximum_error > 12) { std::fputs("JPEG colour regression\n",stderr); std::exit(1); }
  }
  assert(EncodeJpeg(detailed,4096).empty());
  CoUninitialize();
#else
  assert(encoded.empty());
#endif
  capture.rgba.pop_back(); assert(EncodeJpeg(capture,4096).empty());
}
