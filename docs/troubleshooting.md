# Troubleshooting

This page collects the common problems that are easiest to hit while configuring, building, or running the project.

## Submodules

If configuration or build steps fail because dependencies are missing, refresh the submodules.

```bash
git submodule update --init --recursive
```

## Generator Switches

If you reuse a build directory with a different preset family or generator, reconfigure from a clean build tree.

## Managed Build Problems

If the scripting target cannot find the managed toolchain, make sure the .NET SDK 9.0.x is installed and available in PATH.

## Headless Linux

If native tests or editor launch steps fail in headless Linux environments, install the required Mesa and X11 packages and use xvfb-based test execution.

## Runtime Issues

If the editor starts but play mode or scripting behavior looks wrong, check the scripting docs, the asset pipeline, and the latest build output before assuming a code regression.

## What Not To Do

- Do not mix presets inside the same build directory.
- Do not treat generated files or build artifacts as source of truth.
- Do not use comments or docs to hide unclear ownership boundaries; fix the boundary or document it directly.