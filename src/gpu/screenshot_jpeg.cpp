/**
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#include "gpu/screenshot.h"
#include <algorithm>
#include <cstring>
#if defined(_WIN32)
#include <wincodec.h>
#include <wrl/client.h>
#include <wrl/implements.h>
#endif
namespace bd::gpu {
#if defined(_WIN32)
namespace {
// Separate actual written extent from the seek cursor. WIC may seek backward
// after writing the JPEG tail; neither that cursor nor buffer capacity is EOF.
class BoundedJpegStream final : public Microsoft::WRL::RuntimeClass<
    Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>, IStream> {
public:
  explicit BoundedJpegStream(std::vector<uint8_t> &bytes) : bytes_(bytes) {
    std::fill(bytes_.begin(),bytes_.end(),0);
  }
  size_t Size() const { return size_; }
  bool Overflowed() const { return overflowed_; }
  HRESULT STDMETHODCALLTYPE Read(void *data, ULONG count, ULONG *read) override {
    if (read) *read = 0;
    if (!data && count) return STG_E_INVALIDPOINTER;
    const auto available = position_ < size_ ? size_-position_ : 0;
    const auto amount = (std::min)(size_t(count),available);
    if (amount) std::memcpy(data,bytes_.data()+position_,amount);
    position_ += amount;
    if (read) *read = ULONG(amount);
    return amount == count ? S_OK : S_FALSE;
  }
  HRESULT STDMETHODCALLTYPE Write(const void *data, ULONG count, ULONG *written) override {
    if (written) *written = 0;
    if (!data && count) return STG_E_INVALIDPOINTER;
    if (position_ > bytes_.size() || count > bytes_.size()-position_) {
      overflowed_ = true; return STG_E_MEDIUMFULL;
    }
    if (count) {
      if (position_ > size_) std::fill(bytes_.begin()+size_,bytes_.begin()+position_,0);
      std::memcpy(bytes_.data()+position_,data,count);
      position_ += count; size_ = (std::max)(size_,position_);
    }
    if (written) *written = count;
    return S_OK;
  }
  HRESULT STDMETHODCALLTYPE Seek(LARGE_INTEGER move, DWORD origin, ULARGE_INTEGER *position) override {
    const int64_t base = origin == STREAM_SEEK_SET ? 0 : origin == STREAM_SEEK_CUR ? int64_t(position_) : int64_t(size_);
    if (origin > STREAM_SEEK_END || move.QuadPart < -base || move.QuadPart > int64_t(bytes_.size())-base)
      return STG_E_INVALIDFUNCTION;
    position_ = size_t(base+move.QuadPart);
    if (position) position->QuadPart = position_;
    return S_OK;
  }
  HRESULT STDMETHODCALLTYPE SetSize(ULARGE_INTEGER size) override {
    if (size.QuadPart > bytes_.size()) { overflowed_ = true; return STG_E_MEDIUMFULL; }
    if (size.QuadPart > size_) std::fill(bytes_.begin()+size_,bytes_.begin()+size.QuadPart,0);
    size_ = size_t(size.QuadPart); return S_OK;
  }
  HRESULT STDMETHODCALLTYPE Stat(STATSTG *stat, DWORD) override {
    if (!stat) return STG_E_INVALIDPOINTER;
    *stat = {}; stat->type = STGTY_STREAM; stat->cbSize.QuadPart = size_; stat->grfMode = STGM_READWRITE;
    return S_OK;
  }
  HRESULT STDMETHODCALLTYPE Commit(DWORD) override { return S_OK; }
  HRESULT STDMETHODCALLTYPE Revert() override { return E_NOTIMPL; }
  HRESULT STDMETHODCALLTYPE CopyTo(IStream *, ULARGE_INTEGER, ULARGE_INTEGER *, ULARGE_INTEGER *) override { return E_NOTIMPL; }
  HRESULT STDMETHODCALLTYPE LockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD) override { return E_NOTIMPL; }
  HRESULT STDMETHODCALLTYPE UnlockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD) override { return E_NOTIMPL; }
  HRESULT STDMETHODCALLTYPE Clone(IStream **stream) override { if (stream) *stream = nullptr; return E_NOTIMPL; }
private:
  std::vector<uint8_t> &bytes_;
  size_t position_ = 0, size_ = 0;
  bool overflowed_ = false;
};
} // namespace
#endif
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
    auto stream = Microsoft::WRL::Make<BoundedJpegStream>(encoded);
    ComPtr<IWICBitmapEncoder> encoder;
    ComPtr<IWICBitmapFrameEncode> frame;
    ComPtr<IPropertyBag2> properties;
    if (!stream ||
        FAILED(factory->CreateEncoder(GUID_ContainerFormatJpeg,nullptr,&encoder)) ||
        FAILED(encoder->Initialize(stream.Get(),WICBitmapEncoderNoCache)) ||
        FAILED(encoder->CreateNewFrame(&frame,&properties))) return {};
    PROPBAG2 option{}; option.pstrName = const_cast<wchar_t *>(L"ImageQuality");
    VARIANT value{}; value.vt = VT_R4; value.fltVal = quality;
    // Diagnostic material colours must not mix across adjacent pixel blocks.
    PROPBAG2 subsampling{}; subsampling.pstrName = const_cast<wchar_t *>(L"JpegYCrCbSubsampling");
    VARIANT sampling{}; sampling.vt = VT_UI1; sampling.bVal = WICJpegYCrCbSubsampling444;
    WICPixelFormatGUID format = GUID_WICPixelFormat24bppBGR;
    if (FAILED(properties->Write(1,&option,&value)) || FAILED(properties->Write(1,&subsampling,&sampling)) ||
        FAILED(frame->Initialize(properties.Get())) ||
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
    if (!fit || FAILED(frame->Commit()) || FAILED(encoder->Commit()) || stream->Overflowed()) continue;
    const auto written = stream->Size();
    if (written < 4 || written > maximum_bytes || encoded[0] != 255 || encoded[1] != 216 ||
        encoded[written-2] != 255 || encoded[written-1] != 217) return {};
    // Release every stream consumer before moving its backing storage.
    properties.Reset(); frame.Reset(); encoder.Reset(); stream.Reset();
    encoded.resize(written);
    return encoded;
  }
#endif
  return {};
}
} // namespace bd::gpu
