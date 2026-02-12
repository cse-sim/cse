:: mkdocs shortcuts

@echo off

pushd \cse\doc\src

if /I NOT '%1' == 'start' goto notstart
start uv run mkdocs serve --open
goto done
:notstart

if /I NOT '%1' == 'build' goto notbuild
uv run mkdocs build
goto done
:notbuild

echo Unknown choice: '%1'

:done
popd




