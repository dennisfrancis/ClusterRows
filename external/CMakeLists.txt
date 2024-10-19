include(FetchContent)
FetchContent_Declare(
    Eigen
    GIT_REPOSITORY https://gitlab.com/libeigen/eigen.git
    GIT_TAG 3147391d946bb4b6c68edd901f2add6ac1f31f8c  # 3.4.0
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE)

add_subdirectory(eigen)
