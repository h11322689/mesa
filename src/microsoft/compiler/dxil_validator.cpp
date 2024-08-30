   #include "dxil_validator.h"
      #include <windows.h>
   #include <unknwn.h>
      #include "util/ralloc.h"
   #include "util/u_debug.h"
   #include "util/compiler.h"
      #include "dxcapi.h"
      #include <wrl/client.h>
   using Microsoft::WRL::ComPtr;
      struct dxil_validator {
      HMODULE dxil_mod;
            IDxcValidator *dxc_validator;
   IDxcLibrary *dxc_library;
               };
      extern "C" {
   extern IMAGE_DOS_HEADER __ImageBase;
   }
      static HMODULE
   load_dxil_mod()
   {
         #if defined(_GAMING_XBOX_SCARLETT)
         #elif defined (_GAMING_XBOX)
         #else
         #endif
      if (mod)
            /* If that fails, try to load it next to the current module, so we can
   * ship DXIL.dll next to the GLon12 DLL.
            char self_path[MAX_PATH];
   uint32_t path_size = GetModuleFileNameA((HINSTANCE)&__ImageBase,
         if (!path_size || path_size == sizeof(self_path)) {
      debug_printf("DXIL: Unable to get path to self");
               auto last_slash = strrchr(self_path, '\\');
   if (!last_slash) {
      debug_printf("DXIL: Unable to get path to self");
               *(last_slash + 1) = '\0';
   if (strcat_s(self_path, "DXIL.dll") != 0) {
      debug_printf("DXIL: Unable to get path to DXIL.dll next to self");
                  }
      static IDxcValidator *
   create_dxc_validator(HMODULE dxil_mod)
   {
      DxcCreateInstanceProc dxil_create_func =
         if (!dxil_create_func) {
      debug_printf("DXIL: Failed to load DxcCreateInstance from DXIL.dll\n");
               IDxcValidator *dxc_validator;
   HRESULT hr = dxil_create_func(CLSID_DxcValidator,
         if (FAILED(hr)) {
      debug_printf("DXIL: Failed to create validator\n");
                  }
      static enum dxil_validator_version
   get_validator_version(IDxcValidator *val)
   {
      ComPtr<IDxcVersionInfo> version_info;
   if (FAILED(val->QueryInterface(version_info.ReleaseAndGetAddressOf())))
            UINT32 major, minor;
   if (FAILED(version_info->GetVersion(&major, &minor)))
            if (major == 1)
         if (major > 1)
            }
      #ifndef _GAMING_XBOX
   static uint64_t
   get_dll_version(HMODULE mod)
   {
      WCHAR filename[MAX_PATH];
            if (filename_length == 0 || filename_length == ARRAY_SIZE(filename))
            DWORD version_handle = 0;
   DWORD version_size = GetFileVersionInfoSizeW(filename, &version_handle);
   if (version_size == 0)
            void *version_data = malloc(version_size);
   if (!version_data)
            if (!GetFileVersionInfoW(filename, version_handle, version_size, version_data)) {
      free(version_data);
               UINT value_size = 0;
   VS_FIXEDFILEINFO *version_info = nullptr;
   if (!VerQueryValueW(version_data, L"\\", reinterpret_cast<void **>(&version_info), &value_size) ||
      !value_size ||
   version_info->dwSignature != VS_FFI_SIGNATURE) {
   free(version_data);
               uint64_t ret =
      ((uint64_t)version_info->dwFileVersionMS << 32ull) |
      free(version_data);
      }
   #endif
      static enum dxil_validator_version
   get_filtered_validator_version(HMODULE mod, enum dxil_validator_version raw)
   {
         #ifndef _GAMING_XBOX
      case DXIL_VALIDATOR_1_6: {
      uint64_t dxil_version = get_dll_version(mod);
   static constexpr uint64_t known_bad_version =
      // 101.5.2005.60
      if (dxil_version == known_bad_version)
               #endif
      default:
            }
      struct dxil_validator *
   dxil_create_validator(const void *ctx)
   {
      struct dxil_validator *val = rzalloc(ctx, struct dxil_validator);
   if (!val)
            /* Load DXIL.dll. This is a hard requirement on Windows, so we error
   * out if this fails.
   */
   val->dxil_mod = load_dxil_mod();
   if (!val->dxil_mod) {
      debug_printf("DXIL: Failed to load DXIL.dll\n");
               /* Create IDxcValidator. This is a hard requirement on Windows, so we
   * error out if this fails.
   */
   val->dxc_validator = create_dxc_validator(val->dxil_mod);
   if (!val->dxc_validator)
            val->version = get_filtered_validator_version(
      val->dxil_mod,
         /* Try to load dxcompiler.dll. This is just used for diagnostics, and
   * will fail on most end-users install. So we do not error out if this
   * fails.
   */
   val->dxcompiler_mod = LoadLibraryA("dxcompiler.dll");
   if (val->dxcompiler_mod) {
      /* If we managed to load dxcompiler.dll, but either don't find
   * DxcCreateInstance, or fail to create IDxcLibrary or
   * IDxcCompiler, this is a good indication that the user wants
   * diagnostics, but something went wrong. Print warnings to help
   * figuring out what's wrong, but do not treat it as an error.
   */
   DxcCreateInstanceProc compiler_create_func =
      (DxcCreateInstanceProc)GetProcAddress(val->dxcompiler_mod,
      if (!compiler_create_func) {
      debug_printf("DXIL: Failed to load DxcCreateInstance from "
      } else {
      if (FAILED(compiler_create_func(CLSID_DxcLibrary,
                  if (FAILED(compiler_create_func(CLSID_DxcCompiler,
                              fail:
      if (val->dxil_mod)
            ralloc_free(val);
      }
      void
   dxil_destroy_validator(struct dxil_validator *val)
   {
      if (!val)
            /* if we have a validator, we have these */
   val->dxc_validator->Release();
            if (val->dxcompiler_mod) {
      if (val->dxc_library)
            if (val->dxc_compiler)
                           }
      class ShaderBlob : public IDxcBlob {
   public:
      ShaderBlob(void *data, size_t size) :
      m_data(data),
      {
            LPVOID STDMETHODCALLTYPE
   GetBufferPointer(void) override
   {
                  SIZE_T STDMETHODCALLTYPE
   GetBufferSize() override
   {
                  HRESULT STDMETHODCALLTYPE
   QueryInterface(REFIID, void **) override
   {
                  ULONG STDMETHODCALLTYPE
   AddRef() override
   {
                  ULONG STDMETHODCALLTYPE
   Release() override
   {
                  void *m_data;
      };
      bool
   dxil_validate_module(struct dxil_validator *val, void *data, size_t size, char **error)
   {
      if (!val)
                     ComPtr<IDxcOperationResult> result;
   val->dxc_validator->Validate(&source, DxcValidatorFlags_InPlaceEdit,
            HRESULT hr;
            if (FAILED(hr) && error) {
      /* try to resolve error message */
   *error = NULL;
   if (!val->dxc_library) {
      debug_printf("DXIL: validation failed, but lacking IDxcLibrary"
                              if (FAILED(result->GetErrorBuffer(&blob)))
         else if (FAILED(val->dxc_library->GetBlobAsUtf8(blob.Get(),
               else {
      char *str = reinterpret_cast<char *>(blob_utf8->GetBufferPointer());
   str[blob_utf8->GetBufferSize() - 1] = 0;
                     }
      char *
   dxil_disasm_module(struct dxil_validator *val, void *data, size_t size)
   {
      if (!val)
            if (!val->dxc_compiler || !val->dxc_library) {
      fprintf(stderr, "DXIL: disassembly requires IDxcLibrary and "
                     ShaderBlob source(data, size);
            if (FAILED(val->dxc_compiler->Disassemble(&source, &blob))) {
      fprintf(stderr, "DXIL: IDxcCompiler::Disassemble() failed\n");
               if (FAILED(val->dxc_library->GetBlobAsUtf8(blob.Get(), blob_utf8.GetAddressOf()))) {
      fprintf(stderr, "DXIL: IDxcLibrary::GetBlobAsUtf8() failed\n");
               char *str = reinterpret_cast<char*>(blob_utf8->GetBufferPointer());
   str[blob_utf8->GetBufferSize() - 1] = 0;
      }
      enum dxil_validator_version
   dxil_get_validator_version(struct dxil_validator *val)
   {
         }
