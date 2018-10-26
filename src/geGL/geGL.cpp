#include<geGL/OpenGL.h>
#include<geGL/DefaultLoader.h>
#include<cassert>
#include<string>

namespace ge{
  namespace gl{
    GEGL_EXPORT void init(std::shared_ptr<FunctionLoaderInterface>const&loader){
      auto table = ge::gl::createTable(loader);
      ge::gl::setDefaultFunctionTable(table);
      ge::gl::setDefaultContext(ge::gl::createContext(table));
    }
    GEGL_EXPORT void init(GET_PROC_ADDRESS getProcAddress){
      auto loader = std::make_shared<DefaultLoader>(getProcAddress);
      init(loader);
    }


#if defined(_MSC_VER)
#define WIN32_LEAN_AND_MEAN
#include<Windows.h>
    class GEGL_EXPORT OpenGLFunctionLoader{
      protected:
        bool _triedToLoadOpenGL = false;
        bool _triedToLoadGetProcAddress = false;
        HMODULE _openglLib = nullptr;
        using PROC = int(*)();
        using WGLGETPROCADDRESS = PROC(__stdcall*)(LPCSTR);
        WGLGETPROCADDRESS _wglGetProcAddress = nullptr;
      public:
        void*operator()(char const*name){
          assert(this != nullptr);
          const std::string libName = "opengl32.dll";
          const std::string getProcAddressName = "wglGetProcAddress";
          if (!this->_triedToLoadOpenGL){
            this->_triedToLoadOpenGL = true;
            this->_openglLib = LoadLibrary(TEXT(libName.c_str()));
          }
          if (!this->_triedToLoadGetProcAddress){
            this->_triedToLoadGetProcAddress = true;
            if (this->_openglLib)
              this->_wglGetProcAddress = (WGLGETPROCADDRESS)GetProcAddress(this->_openglLib, TEXT(getProcAddressName.c_str()));
            else throw std::runtime_error(std::string("geGL::OpenGLFunctionLoader::operator() - cannot open ") + libName);
          }
          if (!this->_wglGetProcAddress){
            throw std::runtime_error(std::string("geGL::OpenGLFunctionLoader::operator() - cannot load ") + getProcAddressName);
            return nullptr;
          }
          auto ret = (void*)this->_wglGetProcAddress(name);
          if (ret) return ret;
          return (void*)GetProcAddress(this->_openglLib, TEXT(name));
        }
        ~OpenGLFunctionLoader(){
          if (this->_openglLib)FreeLibrary(this->_openglLib);
          this->_openglLib = nullptr;
        }
    };
#else
      
#include <geGL/Dylib.h>
      
#if __APPLE__
      class GEGL_EXPORT OpenGLFunctionLoader {
      protected:
          bool _triedToLoadOpenGL = false;
          void* openglLib = nullptr;
      public:
          void* operator() (char const*name) {
              if (!this->_triedToLoadOpenGL) {
                  
                  /* Load OpenGL framework at default framework search directories.
                   (Default OpenGL path: '/System/Library/Frameworks/OpenGL.framework') */
                  const std::string frameworkName = "OpenGL";
                  this->openglLib = dl_open_framework(frameworkName.c_str());
                  
                  this->_triedToLoadOpenGL = true;
              }
              /* GL functions on OSX have been weak linked since OSX 10.2; this means that
               you can call them directly and unimplemented extensions will resolve to NULL.
               Note that this means that you must parse the extension string to determine if
               a function is valid or not, or your program will crash.
               Source: https://www.khronos.org/opengl/wiki/Load_OpenGL_Functions#MacOSX
               */
              
              // TODO: glGetProcAddress (SDL?)
              return dl_symbol(this->openglLib, name);
          }
          ~OpenGLFunctionLoader() {
              if(openglLib) dl_close_lib(this->openglLib);
              this->openglLib = nullptr;
          }
      };
#else
      class GEGL_EXPORT OpenGLFunctionLoader{
      protected:
          bool _triedToLoadOpenGL = false;
          bool _triedToLoadGetProcAddress = false;
          void*openglLib = nullptr;
          using PROC = void(*)();
          using GETPROCTYPE = PROC(*)(uint8_t const*);
          GETPROCTYPE _glXGetProcAddress = nullptr;
      public:
          void*operator()(char const*name){
              const std::string libName = "libGL.so.1";
              const std::string getProcAddressName = "glXGetProcAddress";
              if(!this->_triedToLoadOpenGL){
                  this->_triedToLoadOpenGL = true;
                  this->openglLib = dl_open_lib(libName.c_str());
              }
              if(!this->_triedToLoadGetProcAddress){
                  this->_triedToLoadGetProcAddress = true;
                  if(this->openglLib)
                      reinterpret_cast<void*&>(this->_glXGetProcAddress) = dl_symbol(this->openglLib,getProcAddressName.c_str());
                  else throw std::runtime_error("geGL::OpenGLFunctionLoader::operator() - cannot open "+libName);
              }
              if(!this->_glXGetProcAddress){
                  throw std::runtime_error("geGL::OpenGLFunctionLoader::operator() - cannot load " + getProcAddressName);
                  return nullptr;
              }
              return (void*)this->_glXGetProcAddress((uint8_t const*)(name));
          }
          ~OpenGLFunctionLoader(){
              if(openglLib)dl_close_lib(this->openglLib);
              this->openglLib = nullptr;
          }
      };
#endif
#endif

    GEGL_EXPORT void*getProcAddress(char const*name){
      static OpenGLFunctionLoader loader{};
      return loader(name);
    }
  }
}


