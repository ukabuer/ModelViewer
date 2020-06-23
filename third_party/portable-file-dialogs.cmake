include_guard()

include(FetchContent)

FetchContent_Declare(
    portable_file_dialogs
    GIT_REPOSITORY https://github.com/samhocevar/portable-file-dialogs.git
    GIT_TAG f18ca871f60ad0470e253d193b9519c58e2745c4
    GIT_PROGRESS TRUE
)

FetchContent_MakeAvailable(portable_file_dialogs)
