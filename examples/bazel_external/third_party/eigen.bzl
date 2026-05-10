def _impl(ctx):
    _rix_path = ctx.os.environ.get("HOME", "") + "/.rix"

    ctx.symlink(_rix_path + "/include/eigen3", "include/eigen3")
    ctx.symlink(Label("@//:third_party/eigen.BUILD.bazel"), "BUILD.bazel")

local_eigen_repository = repository_rule(
    implementation = _impl,
)
