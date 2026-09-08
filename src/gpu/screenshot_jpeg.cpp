/**
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#include "gpu/screenshot.h"
#if defined(_WIN32)
#include <wincodec.h>
#include <wrl/client.h>
#endif
namespace bd::gpu {
std::vector<uint8_t> EncodeJpeg(const Capture &capture, size_t maximum_bytes) {
  if (!capture.width || !capture.height || capture.width > 2048 || capture.height > 1200 ||
      capture.rgba.size() != uint64_t(capture.width)*capture.height*4 ||
      !maximum_bytes || maximum_bytes > (1u << 20)) return {};
#if defined(_WIN32)
  using Microsoft::WRL::ComPtr;
  const HRESULT initialized = CoInitializeEx(nullptr,COINIT_MULTITHREADED);
  if (FAILED(initialized) && initialized != RPC_E_CHANGED_MODE) return {};
  struct Apartment { bool owned; ~Apartment() { if (owned) CoUninitialize(); } } apartment{SUCCEEDED(initialized)};
  ComPtr<IWICImagingFactory> factory;
  if (FAILED(CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&factory)))) return {};
  std::vector<uint8_t> row(capture.width*3), encoded(maximum_bytes);
  // Fixed-capacity memory stream: the encoder cannot grow a file or memory
  // stream past the caller's limit. Retry quality only, never image dimensions.
  for (float quality : {.60f,.50f,.40f,.30f}) {
    ComPtr<IWICStream> stream;
    ComPtr<IWICBitmapEncoder> encoder;
    ComPtr<IWICBitmapFrameEncode> frame;
    ComPtr<IPropertyBag2> properties;
    if (FAILED(factory->CreateStream(&stream)) ||
        FAILED(stream->InitializeFromMemory(encoded.data(),DWORD(encoded.size()))) ||
        FAILED(factory->CreateEncoder(GUID_ContainerFormatJpeg,nullptr,&encoder)) ||
        FAILED(encoder->Initialize(stream.Get(),WICBitmapEncoderNoCache)) ||
        FAILED(encoder->CreateNewFrame(&frame,&properties))) return {};
    PROPBAG2 option{}; option.pstrName = const_cast<wchar_t *>(L"ImageQuality");
    VARIANT value{}; value.vt = VT_R4; value.fltVal = quality;
    WICPixelFormatGUID format = GUID_WICPixelFormat24bppBGR;
    if (FAILED(properties->Write(1,&option,&value)) || FAILED(frame->Initialize(properties.Get())) ||
        FAILED(frame->SetSize(capture.width,capture.height)) || FAILED(frame->SetPixelFormat(&format)) ||
        format != GUID_WICPixelFormat24bppBGR) return {};
    bool fit = true;
    for (uint32_t y=0;y<capture.height && fit;++y) {
      const auto *input = capture.rgba.data()+uint64_t(y)*capture.width*4;
      for (uint32_t x=0;x<capture.width;++x) {
        row[x*3] = input[x*4+2]; row[x*3+1] = input[x*4+1]; row[x*3+2] = input[x*4];
      }
      fit = SUCCEEDED(frame->WritePixels(1,UINT(row.size()),UINT(row.size()),row.data()));
    }
    if (!fit || FAILED(frame->Commit()) || FAILED(encoder->Commit())) continue;
    ULARGE_INTEGER written{};
    if (FAILED(stream->Seek({},STREAM_SEEK_CUR,&written)) || !written.QuadPart || written.QuadPart > maximum_bytes) return {};
    encoded.resize(size_t(written.QuadPart));
    return encoded;
  }
#endif
  return {};
}
} // namespace bd::gpu
