#include <GwongDongFileSystem.hpp>


namespace GwongDongFileSystem
{
    LogCallBack FSLogCallback;
    void SetLogCallback(LogCallBack callback)
    {
        FSLogCallback = callback;
    }

}
