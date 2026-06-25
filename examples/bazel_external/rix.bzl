def _impl(ctx):
    _rix_path = ctx.os.environ.get("HOME", "") + "/.rix"

    ctx.symlink(_rix_path + "/lib", "lib")
    ctx.symlink(_rix_path + "/include", "include")
    ctx.symlink(Label("@//:rix.BUILD.bazel"), "BUILD.bazel")

local_rix_repository = repository_rule(
    implementation = _impl,
)
