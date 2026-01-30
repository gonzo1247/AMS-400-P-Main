include_guard()
include(CopyUtils)

function(copy_zxing_dlls target)
    message(STATUS "ZXing is statically linked — no DLLs to copy.")
endfunction()